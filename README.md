# atlas

Atlas is a small interpreted programming language written in C.

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

- Numeric literals (stored as `double`)
- Variable declaration with `let`
- Assignment to existing variables
- Arithmetic expressions with precedence
- `print` statement

Example:

```atlas
let x = 40;
let y = 2;
print x + y;
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
