# Atlas Language Reference

Atlas is a small interpreted language implemented in C.

## Syntax

Every statement ends with `;`.

Atlas is globe-first: code runs only by igniting globes.

### Globes

Define a globe using braces:

```atlas
globe origin(x, y) {
	echo x + y;
}
```

Run it with `ignite`:

```atlas
ignite origin(20, 22);
```

### Returns

Globes can return values:

```atlas
globe sum(a, b) {
	return a + b;
}

globe caller() {
	seed answer <- sum(20, 22);
	echo answer;
}
```

### Seeds (Variables)

Create a seed with `seed` and `<-`:

```atlas
seed x <- 10;
seed y <- x + 2;
```

Rebind an existing seed:

```atlas
x <- x + 1;
```

### Booleans

Atlas supports boolean literals:

```atlas
seed enabled <- true;
seed muted <- false;
echo enabled;
```

### Loops With `orbit`

`orbit(condition) { ... }` repeats while the condition remains truthy.

```atlas
seed countdown <- 3;
orbit(countdown) {
	echo countdown;
	countdown <- countdown - 1;
}
```

Loop control keywords:

- `break;` exits the nearest `orbit`
- `continue;` skips to the next `orbit` iteration

### OO Design With Molds

Define object blueprints with `mold`:

```atlas
mold Beacon {
	seed active <- true;
	seed watts <- 12;
}
```

Create object instances with `craft`:

```atlas
seed b <- craft(Beacon);
echo b.active;
b.watts <- b.watts + 8;
echo b.watts;
```

Inheritance:

```atlas
mold Animal {
	seed name <- as_text(0);
}

mold Dog <- Animal {
	seed power <- 0;
}
```

Constructors:

- Define constructor method as `<Mold>_construct(self, ...)`
- It runs automatically on `craft(Mold, ...)`

Polymorphism (override methods by mold name):

```atlas
globe Animal_voice(self) {
	return 1;
}

globe Dog_voice(self) {
	return 2;
}

globe Dog_construct(self, name, power) {
	self.name <- name;
	self.power <- power;
	return 0;
}

seed pet <- craft(Dog, as_text(77), 9);
echo pet.voice();
```

### Dictionaries

Atlas provides dictionary-like dynamic maps through built-ins:

- `vault()` creates a dictionary
- `stash(dict, key, value)` inserts or updates
- `pull(dict, key)` reads a value
- `has(dict, key)` checks key existence
- `drop(dict, key)` removes a key

Keys are converted to text internally.

```atlas
seed bag <- vault();
seed key <- as_text(42);
stash(bag, key, 9001);
echo has(bag, key);
echo pull(bag, key);
drop(bag, key);
```

### Echo

```atlas
echo x;
echo x + y;
```

### Types And Conversion

Introspect values:

```atlas
echo kind(42);        // number
echo kind(true);      // boolean
echo kind(craft(Beacon)); // object
```

Convert values:

```atlas
echo as_number(true);   // 1
echo as_bool(0);        // false
echo as_text(12.5);     // "12.5"
```

### Expressions

Supported operators:

- `+` add
- `-` subtract or unary negation
- `*` multiply
- `/` divide
- parentheses `(` `)`

Operator precedence:

1. Parentheses
2. Unary `-`
3. `*` and `/`
4. `+` and `-`

## Notes

- Seeds can store number, boolean, text, and object values.
- Rebinding requires the seed to already exist.
- Division by zero is a runtime error.
- Top-level code allows `globe`, `mold`, and `ignite` declarations.
