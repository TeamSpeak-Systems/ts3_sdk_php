--TEST--
getting uniqueid from identity
--FILE--
<?php
require dirname(__DIR__)."/test_server.php";
$identity = "113V64iwHGLQ/osk+6qTcgmXA66HL29tIxpfeA0MRVtFe1YreB9iBWtFFlodUlNtL1h8Vhp7fkNASiVVQn0Gf3VqVXJbZHdxFVR/UDJjBwk0FyN5Sl98IAVdEhgWdHkISEVYI0d8aXU2RUNJUUNUOVpFSVNqRUZzd3RjWC9xWHZ3Q2R3T3lhWmNLSE54TEZtRXc1R0UwQkVnPT0=";
if (ts3client_identityStringToUniqueIdentifier($identity, $uniqueID) != ERROR_ok)
    exit("failed getting uniqueid of identity");
echo($uniqueID);
?>
--EXPECT--
dJAsrqJlyztYRGvqgpiF69qVIUM=
