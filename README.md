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
- Globe parameters with `globe name(a, b)`
- Globe execution with `ignite name(args...);`
- Globe return values with `return expr;`
- Loops with `orbit(condition) { ... }`
- Loop control with `break;` and `continue;`
- Boolean literals `true` and `false`
- Numeric literals (stored as `double`)
- Seed declaration with `seed name <- expr;`
- Seed rebinding with `name <- expr;`
- Object-oriented molds with `mold` and `craft`
- Mold inheritance with `mold Child <- Parent`
- Constructors via `<Mold>_construct(self, ...)`
- Polymorphic method dispatch via `obj.method(...)`
- Dictionary equivalents with `vault`, `stash`, `pull`, `has`, `drop`
- Field access and mutation with `obj.field` and `obj.field <- ...`
- Runtime type introspection with `kind(value)`
- Type conversion with `as_number`, `as_bool`, and `as_text`
- Arithmetic expressions with precedence
- `echo` statement

Example:

```atlas
globe fuse(x, y) {
	return x + y;
}

globe origin(x, y, enabled) {
	seed total <- fuse(x, y);
	seed bag <- vault();
	stash(bag, as_text(1), total);
	echo total;
	echo pull(bag, as_text(1));
	return enabled;
}

ignite origin(40, 2, true);
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
- Add conditionals and comparison operators
- Add functions
- Add standard library modules
