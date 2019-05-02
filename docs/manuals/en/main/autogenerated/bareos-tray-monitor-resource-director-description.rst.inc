.. config:option:: console/director/Address

   :required: True
   :type: STRING

   .. include:: /config-directive-description/console-director-Address.rst.inc



.. config:option:: console/director/Description

   :type: STRING

   .. include:: /config-directive-description/console-director-Description.rst.inc



.. config:option:: console/director/DirPort

   :type: PINT32
   :default: 9101

   .. include:: /config-directive-description/console-director-DirPort.rst.inc



.. config:option:: console/director/Name

   :required: True
   :type: NAME

   .. include:: /config-directive-description/console-director-Name.rst.inc



.. config:option:: console/director/TlsAllowedCn

   :type: STRING_LIST

   "Common Name"s (CNs) of the allowed peer certificates.

   .. include:: /config-directive-description/console-director-TlsAllowedCn.rst.inc



.. config:option:: console/director/TlsAuthenticate

   :type: BOOLEAN
   :default: no

   Use TLS only to authenticate, not for encryption.

   .. include:: /config-directive-description/console-director-TlsAuthenticate.rst.inc



.. config:option:: console/director/TlsCaCertificateDir

   :type: DIRECTORY

   Path of a TLS CA certificate directory.

   .. include:: /config-directive-description/console-director-TlsCaCertificateDir.rst.inc



.. config:option:: console/director/TlsCaCertificateFile

   :type: DIRECTORY

   Path of a PEM encoded TLS CA certificate(s) file.

   .. include:: /config-directive-description/console-director-TlsCaCertificateFile.rst.inc



.. config:option:: console/director/TlsCertificate

   :type: DIRECTORY

   Path of a PEM encoded TLS certificate.

   .. include:: /config-directive-description/console-director-TlsCertificate.rst.inc



.. config:option:: console/director/TlsCertificateRevocationList

   :type: DIRECTORY

   Path of a Certificate Revocation List file.

   .. include:: /config-directive-description/console-director-TlsCertificateRevocationList.rst.inc



.. config:option:: console/director/TlsCipherList

   :type: DIRECTORY

   List of valid TLS Ciphers.

   .. include:: /config-directive-description/console-director-TlsCipherList.rst.inc



.. config:option:: console/director/TlsDhFile

   :type: DIRECTORY

   Path to PEM encoded Diffie-Hellman parameter file. If this directive is specified, DH key exchange will be used for the ephemeral keying, allowing for forward secrecy of communications.

   .. include:: /config-directive-description/console-director-TlsDhFile.rst.inc



.. config:option:: console/director/TlsEnable

   :type: BOOLEAN
   :default: yes

   Enable TLS support.

   .. include:: /config-directive-description/console-director-TlsEnable.rst.inc



.. config:option:: console/director/TlsKey

   :type: DIRECTORY

   Path of a PEM encoded private key. It must correspond to the specified "TLS Certificate".

   .. include:: /config-directive-description/console-director-TlsKey.rst.inc



.. config:option:: console/director/TlsRequire

   :type: BOOLEAN
   :default: no

   Without setting this to yes, Bareos can fall back to use unencrypted connections. Enabling this implicitly sets "TLS Enable = yes".

   .. include:: /config-directive-description/console-director-TlsRequire.rst.inc



.. config:option:: console/director/TlsVerifyPeer

   :type: BOOLEAN
   :default: no

   If disabled, all certificates signed by a known CA will be accepted. If enabled, the CN of a certificate must the Address or in the "TLS Allowed CN" list.

   .. include:: /config-directive-description/console-director-TlsVerifyPeer.rst.inc


