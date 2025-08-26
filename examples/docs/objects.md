# Objects

```java
object <name>
{
    <required|optional|seperate>:?
    <field>: <type> =? <value>;...


}
```

An object contains three types of fields, required, optional and seperate fields.

required fields are fields that are the most important of the object. They must be provided in the objects
constructor for the object to be created.

```java

object X
{
required:
    y: int;
    z: int;

}
x: X = new(4); // error z not given value.
x: X = new(4, 2); // success!
```

Required fields cannot be given default values. This is because their values must be passed in by the constructor.
 
## Optional fields

Optional is the default state of an object field. It is given to a member when that member could be initialised in the constructor, or could not.

```java
object X
{
required:
    y: int;

optional:
    z: int = 4;
    
}
x: X = new(4); // fine
x: X = new(4, 12); // also fine
x: X = new(z=12); // error no value for y
```

If the optional field is not given a default value, then it will be given the default value of its field.

If the optional field is of an object type `x` and `x` does not have a constructor of 0 arguments, a compilation error is thrown. This is because an optional field must be able to be created on its own.

```java

object Y
{
required:
    x: int;
}

object X
{
    z: Y; // default optional, although converted to required as it cant be created on its own.
}
object Z
{
    x: Y?;
}
object W
{
    x: Y[];
}

test: X = new(); // compile error thrown, missing required argument Z
test: Z = new(); // works fine as the default for an optional type is `null`
test: W = new(); // works fine as the default for an array is an empty array
```

## Seperate fields

Seperate fields are completely seperate from the instantiation process and the constructor.
They are used for fields that are used as part of the object but do not represent the objects core fields.


## Constructing objects

The new keyword is used to construct an object.

```java

object X
{
    field: int;
}

a: X; // compiles

```
If the constructor of an object or type does not require arguments, the new keyword can be omitted.

```java
a: X;
// same as:
a: X = new();
```

You can use `unordered arguments` after all ordered arguments in a constructor to initialize other fields, regardless of their field decorator.

```java
object X
{
required:
    z: int;
optional:
    stuff: float;
seperate:
    extra_hidden_stuff: int;
}
a: X = new(4, 12, extra_hidden_stuff=122);
a: X = new(z=4, stuff=12, extra_hidden_stuff=14);
a: X = new(stuff=12, 4); // error!

```