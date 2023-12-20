# Command line Options `vtm(.exe)`

 `vtm [ -c <file> ] [ -p <pipe> ] [ -i | -u ] [ -q ] [ -l | -m | -d | -s | -r [<app> [<args...>]] ]`

Option                       | Description
-----------------------------|-------------------------------------------------------
No arguments                 | Run client (auto start server).
` -c `, ` --config <file> `  | Use specified configuration file.
` -p `, ` --pipe <pipe> `    | Set the pipe to connect to.
` -q `, ` --quiet `          | Disable logging.
` -l `, ` --listconfig `     | Show configuration.
` -m `, ` --monitor `        | Monitor server session log.
` -d `, ` --daemon `         | Run server in background.
` -s `, ` --server `         | Run server in interactive mode.
` -r `, ` --runapp [<app>] ` | Run the specified `<app>` standalone.
` -i `, ` --install `        | System-wide installation.
` -u `, ` --uninstall `      | System-wide deinstallation.
` -v `, ` --version `        | Show version.
` -? `, ` -h `, ` --help `   | Show usage message.
` --onlylog  `               | Disable interactive user input.

Registered `<app>`s:

Application | Description
------------|------------------------------------------
`Term`      | Terminal emulator (default)
`DTVT`      | DirectVT Proxy Console
`XLVT`      | DirectVT Proxy Console with controlling terminal onboard (Cross-linked VT)
`Calc`      | Spreadsheet calculator (Demo)
`Text`      | Text editor (Demo)
`Gems`      | Application manager (Demo)
`Test`      | Test page (Demo)
`Truecolor` | Test page (Demo)