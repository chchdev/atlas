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

- Seeds are numeric (`double`) values.
- Seeds can store number, boolean, text, and object values.
- Rebinding requires the seed to already exist.
- Division by zero is a runtime error.
- Top-level code allows `globe`, `mold`, and `ignite` declarations.
