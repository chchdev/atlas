# Atlas Language Reference

Atlas is a small interpreted language implemented in C.

## Syntax

Every statement ends with `;`.

### Variables

Declare with `let`:

```atlas
let x = 10;
let y = x + 2;
```

Assign to an existing variable:

```atlas
x = x + 1;
```

### Print

```atlas
print x;
print x + y;
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

- Variables are numeric (`double`) values.
- Assignment requires the variable to already exist.
- Division by zero is a runtime error.
