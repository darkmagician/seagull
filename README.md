a copy of http://gull.sourceforge.net/

Fixed a radius authentication problem in *libcrypto.so*.
Use *build.libcrypto.conf* to replace *build.conf* for libcrypto.so build (windows support is also added.). The avaiable build is under *release-libcrypto*



The message is built with the "Authenticator" field filled with *0x00000000000000000000000000000000* in the scenario. (Don't lose any zeros. There must be 32 zeros (16 bytes)) 
```xml
<Message name="Accounting-Request">
      <setfield name="Authenticator" value="0x00000000000000000000000000000000"> </setfield>
      ...
```
In the pre-action part of the scenario, the "Authenticator" field is set with the result of the authentication method: 
```xml
<set-value name="Authenticator" method="authentication" message_part="all"
                 format="shared_secret=REPLACE_WITH_SHARED_SECRET"></set-value>
```
please set REPLACE_WITH_SHARED_SECRET with your shared secret which should be consistent with the server connector.

The authentication method must be defined in the dictionary with its name, the name of the function to be used and the name of the library in which the function is defined. Example: 
```xml
<!-- external methods for fields modifications -->
<external-method>
        <defmethod name="authentication"
                   param="lib=lib_crypto.so;function=crypto_method_radius">
        </defmethod>
</external-method> 
```
lib_crypto.so is the library from the patch above. Please make the directory where it locates included in LD_LIBRARY_PATH.



