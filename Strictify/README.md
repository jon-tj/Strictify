# Strictify

Enforce stricter rules on notes!

## Compile

```shell
cmake -S . -B build; cmake --build build; .\build\Debug\strictify.exe ".." "..\Example\src"
```

## Rules

### Cards

Card elements (`<card>`) must have exactly one of either `<fixed>` or `<front>` as well as exactly one `<back>`.