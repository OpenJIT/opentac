f: (out i32, in i32) -> unit;
f :: (out p0: i32, in p1: i32) => {
  t0 := add i32, p1, 1:i32;
  p0[0:u32] := t1;
  return unit, unit;
}

main: () -> i32;
main :: () => {
  t0 := alloca u64, 8, 8;
  t1 := add i32, 42:i32, 69:i32;
  return i32, t1;
}
