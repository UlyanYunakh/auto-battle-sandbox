# Project Coding Rules

- For Unreal C++ classes, keep access sections ordered as `public:`, then `protected:`, then `private:`.
- Separate methods and properties into their own access sections, even when that repeats the same access specifier. For
  example, use one `protected:` block for overrides/methods and a later `protected:` block for `UPROPERTY` members.
