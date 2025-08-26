type short = int;
type long  = int;

module types
{
    /*
    Converts a type to its string representation at compile time.

    Note: if this function is passed a generic type, the type passed to that generic type
          will be passed to this function.
    */
    method<_Type>: string to_string() __cpp__ __nocompile__;
}