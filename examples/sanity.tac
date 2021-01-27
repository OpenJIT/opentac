f: (i32) -> i32;
f :: (p0: i32) => {
  t0 := add i32, p0, 1:i32;
  return i32, t0;
}

main: () -> i32;
main :: () => {
  t0 := alloca u64, 8, 8;
  t1 := add i32, 42:i32, 69:i32;
  return i32, t1;
}
