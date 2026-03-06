# atlas

Atlas is a globe-driven interpreted programming language written in C.

## Project Structure

```text
atlas/
	include/atlas/
		ast.h
		lexer.h
		parser.h
		runtime.h
		token.h
		util.h
	src/
		ast.c
		lexer.c
		main.c
		parser.c
		runtime.c
		util.c
	examples/
		hello.atlas
		math.atlas
	tests/
		smoke.atlas
		run_tests.ps1
	docs/
		language.md
	CMakeLists.txt
	README.md
```

## Atlas Language

Current Atlas features:

- Globe declarations with `globe name { ... }`
- Globe execution with `ignite name;`
- Numeric literals (stored as `double`)
- Seed declaration with `seed name <- expr;`
- Seed rebinding with `name <- expr;`
- Arithmetic expressions with precedence
- `echo` statement

Example:

```atlas
globe origin {
	seed x <- 40;
	seed y <- 2;
	echo x + y;
}

ignite origin;
```

## Build

From the repository root:

```powershell
cmake -S . -B build
cmake --build build
```

## Run

Run a script file:

```powershell
.\build\atlas.exe .\examples\hello.atlas
```

Run REPL mode:

```powershell
.\build\atlas.exe
```

## Test

After building:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_tests.ps1
```

## Next Ideas

- Add booleans and comparison operators
- Add conditionals and loops
- Add functions
- Add standard library modules
