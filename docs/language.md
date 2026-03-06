# Atlas Language Reference

Atlas is a small interpreted language implemented in C.

## Syntax

Every statement ends with `;`.

Atlas is globe-first: code runs only by igniting globes.

### Globes

Define a globe using braces:

```atlas
globe origin {
	echo 42;
}
```

Run it with `ignite`:

```atlas
ignite origin;
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

### Echo

```atlas
echo x;
echo x + y;
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
- Rebinding requires the seed to already exist.
- Division by zero is a runtime error.
- Top-level code only allows `globe` declarations and `ignite` calls.
