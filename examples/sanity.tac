f: (i32) -> i32;
f :: (p0: i32) => {
  t0 := add p0, 1:i32;
  return t0;
}

main: () -> i32;
main :: () => {
  t0 := alloca 8, 8;
  t1 := add 42:i32, 69:i32;
  return t1;
}
