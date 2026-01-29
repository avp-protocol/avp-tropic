# 4. Full Chain Verification Example

In this tutorial, you will learn about one of the steps required to verify the authenticity of the TROPIC01 certificate chain — a process that should be done by Tropic Square customers during provisioning of their device which integrates TROPIC01.

!!! info "Important: read before proceeding"
    Detailed information about TROPIC01 device identity and related Tropic Square Public Key Infrastructure (PKI) is provided in the *Device Identity and PKI Application Note* (ODN_TR01_app_003) available on [GitHub](https://github.com/tropicsquare/tropic01/tree/main#application-notes). It is recommended to read this document before proceeding to understand the process described in this tutorial, as here we provide only minimal information to try the process of the certificate chain verification.

!!! warning "Compatibility"
    Only production chips contain full certificate chain. Sample chips are not supported by this tutorial. If you encounter problems in this tutorial, you probably have an incompatible chip. Find your TROPIC01's part number ([check the FAQ](../../../faq.md#what-is-the-part-number-pn-of-my-tropic01)) and check the [Catalog list](https://github.com/tropicsquare/tropic01#available-parts) to see if your chip is a production one.

The TROPIC01 comes with its own unique cryptographic identity in the form of Secure Channel key pair and a certificate. The certificate is issued by Tropic Square PKI which provides a framework for verifying the origin of each TROPIC01 chip ever produced. In this tutorial, we will learn:

- How to load the certificate chain from a TROPIC01 chip using `lt_get_info_cert_store()` function from the Libtropic API.
- How to verify all certificates in the chain using OpenSSL CLI with a provided script.

## Load the Certificates
First, we will load the certificates from your TROPIC01 using a provided C application available in `examples/linux/usb_devkit/full_chain_verification`.

!!! example "Building and running the app"
    === ":fontawesome-brands-linux: Linux"
        Go to the example's project directory:
        ```bash { .copy }
        cd examples/linux/usb_devkit/full_chain_verification/
        ```

        Create a `build/` directory and switch to it:
        ```bash { .copy }
        mkdir build/
        cd build/
        ```

        And finally, build and run the application:
        ```bash { .copy }
        cmake ..
        make
        ./libtropic_dump_certificates
        ```

        The certificates will be stored in the build directory. If the application completed execution without any errors, you should see the following certificates in the build directory:
        
        - `t01_ca_cert.der`
        - `t01_ese_cert.der`
        - `t01_xxxx_ca_cert.der`
        - `tropicsquare_root_ca_cert.der`

    === ":fontawesome-brands-apple: macOS"
        TBA

    === ":fontawesome-brands-windows: Windows"
        TBA


## Verify the Certificates
After loading the certificates from the TROPIC01 chip, we will verify the certificates using a provided script.

!!! example "Verifying the certificates"
    === ":fontawesome-brands-linux: Linux"
        Go to the example's project directory:
        ```bash { .copy }
        cd examples/linux/usb_devkit/full_chain_verification/
        ```

        Run the `verify.sh` script. It accepts a single argument: a path to a directory where certificates that we loaded from the TROPIC01 are located.
        ```bash { .copy }
        ./verify.sh build/
        ```

        If all certificates are verified successfully, the script will return 0 and output the following message: `All certificates verified successfully!`

    === ":fontawesome-brands-apple: macOS"
        TBA

    === ":fontawesome-brands-windows: Windows"
        TBA


### Understanding the Script
The script demonstrates almost all important steps in the verification process:

1. Download revocation lists from the URLs specified in the certificates which we obtained from the TROPIC01.
2. Check all certificates we obtained from the TROPIC01 using the chain and revocation lists we downloaded from the Tropic Square PKI website.
3. Check the root certificate (simplified, no out-of-band check provided).

Authenticity check of the root certificate in the step 3 is not fully implemented. The root certificate can be obtained from the chip, we provide it in this repository and it is also available in the Tropic Square PKI website. **Do not blindly trust this certificate** file from GitHub alone. To protect against repository compromise, the trust has to be established by verifying the certificate fingerprint through an independent channel. Tropic Square customers can obtain the verified fingerprint via direct contact with [Customer Support](https://support.tropicsquare.com).

The script contains comments about each step, so refer to the code of the script for more details about the implementation. It is also recommended to study the *Device Identity and PKI Application Note* (ODN_TR01_app_003) (available [on GitHub](https://github.com/tropicsquare/tropic01/tree/main#application-notes)) to fully understand the principles described in this tutorial.

!!! note "Alternative implementation of the verification"
    The script verifies the TROPIC01 certificates against certificate authority certificates downloaded from the Tropic Square PKI website. As the same certificates are present also in the TROPIC01 itself, those can be used instead. The importance of verifying the root certificate independently remains the key part of the process.


