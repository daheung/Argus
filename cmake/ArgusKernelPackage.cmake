foreach(RequiredVariable IN ITEMS
    ARGUS_PACKAGE_DIR ARGUS_SYS_FILE ARGUS_INF_FILE ARGUS_INF2CAT_EXE
    ARGUS_SIGNTOOL_EXE ARGUS_SIGNING_DIR)
    if(NOT DEFINED ${RequiredVariable} OR "${${RequiredVariable}}" STREQUAL "")
        message(FATAL_ERROR "${RequiredVariable} is required to build ArgusKernelPackage.")
    endif()
endforeach()

set(TestKey "${ARGUS_SIGNING_DIR}/ArgusTest.key")
set(TestCertificate "${ARGUS_SIGNING_DIR}/ArgusTest.cer")
set(TestPfx "${ARGUS_SIGNING_DIR}/ArgusTest.pfx")

# This disposable test key stays under the ignored .build directory. The public
# certificate must be trusted on the test VM before the package can be installed.
if(NOT EXISTS "${TestPfx}" OR NOT EXISTS "${TestCertificate}")
    if(NOT EXISTS "${ARGUS_OPENSSL_EXE}")
        message(FATAL_ERROR "OpenSSL is required once to create the Argus test certificate. Set ARGUS_OPENSSL_EXECUTABLE or install OpenSSL.")
    endif()

    file(MAKE_DIRECTORY "${ARGUS_SIGNING_DIR}")
    execute_process(
        COMMAND "${ARGUS_OPENSSL_EXE}" req -x509 -newkey rsa:3072 -sha256
            -days 3650 -nodes -keyout "${TestKey}" -out "${TestCertificate}"
            -subj "/CN=Argus Test Signing"
            -addext "extendedKeyUsage=codeSigning"
            -addext "keyUsage=digitalSignature"
        RESULT_VARIABLE CertificateResult
        OUTPUT_QUIET ERROR_VARIABLE CertificateError
    )
    if(NOT CertificateResult EQUAL 0)
        message(FATAL_ERROR "Failed to create the test certificate: ${CertificateError}")
    endif()

    execute_process(
        COMMAND "${ARGUS_OPENSSL_EXE}" pkcs12 -export
            -inkey "${TestKey}" -in "${TestCertificate}" -out "${TestPfx}"
            -passout "pass:"
        RESULT_VARIABLE PfxResult
        OUTPUT_QUIET ERROR_VARIABLE PfxError
    )
    if(NOT PfxResult EQUAL 0)
        message(FATAL_ERROR "Failed to create the test PFX: ${PfxError}")
    endif()
    file(REMOVE "${TestKey}")
endif()

file(MAKE_DIRECTORY "${ARGUS_PACKAGE_DIR}")
file(COPY_FILE "${ARGUS_INF_FILE}" "${ARGUS_PACKAGE_DIR}/ArgusKernel.inf" ONLY_IF_DIFFERENT)
file(COPY_FILE "${ARGUS_SYS_FILE}" "${ARGUS_PACKAGE_DIR}/ArgusKernel.sys" ONLY_IF_DIFFERENT)

execute_process(
    COMMAND "${ARGUS_SIGNTOOL_EXE}" sign /fd SHA256 /f "${TestPfx}" /p ""
        "${ARGUS_PACKAGE_DIR}/ArgusKernel.sys"
    RESULT_VARIABLE DriverSignResult
    OUTPUT_VARIABLE DriverSignOutput ERROR_VARIABLE DriverSignError
)
if(NOT DriverSignResult EQUAL 0)
    message(FATAL_ERROR "SignTool could not sign ArgusKernel.sys:\n${DriverSignOutput}\n${DriverSignError}")
endif()

execute_process(
    COMMAND "${ARGUS_INF2CAT_EXE}" "/driver:${ARGUS_PACKAGE_DIR}" /os:10_GE_X64
    RESULT_VARIABLE Inf2CatResult
    OUTPUT_VARIABLE Inf2CatOutput ERROR_VARIABLE Inf2CatError
)
if(NOT Inf2CatResult EQUAL 0)
    message(FATAL_ERROR "Inf2Cat failed:\n${Inf2CatOutput}\n${Inf2CatError}")
endif()

file(GLOB CatalogFiles "${ARGUS_PACKAGE_DIR}/*.cat")
list(LENGTH CatalogFiles CatalogCount)
if(NOT CatalogCount EQUAL 1)
    message(FATAL_ERROR "Expected one CAT file in ${ARGUS_PACKAGE_DIR}, found ${CatalogCount}.")
endif()
list(GET CatalogFiles 0 CatalogFile)

execute_process(
    COMMAND "${ARGUS_SIGNTOOL_EXE}" sign /fd SHA256 /f "${TestPfx}" /p "" "${CatalogFile}"
    RESULT_VARIABLE SignResult
    OUTPUT_VARIABLE SignOutput ERROR_VARIABLE SignError
)
if(NOT SignResult EQUAL 0)
    message(FATAL_ERROR "SignTool failed:\n${SignOutput}\n${SignError}")
endif()

message(STATUS "Signed ArgusKernel package: ${ARGUS_PACKAGE_DIR}")
message(STATUS "Test certificate for the guest: ${TestCertificate}")
