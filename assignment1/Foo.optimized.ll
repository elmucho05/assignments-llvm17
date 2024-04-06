; ModuleID = 'Foo.optimized.bc'
source_filename = "Foo.ll"

define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) {
  %3 = add nsw i32 %1, 1
  %4 = mul nsw i32 %3, 2
  %5 = shl i32 %3, 1
  %6 = shl i32 %0, 1
  %7 = sdiv i32 %6, 4
  %8 = lshr i32 %6, 2
  %9 = mul nsw i32 %5, %8
  %10 = add nsw i32 %0, 1
  %11 = sub nsw i32 %10, 1
  %12 = add nsw i32 %1, 0
  %13 = mul nsw i32 %1, 15
  %14 = shl i32 %1, 4
  %15 = sub i32 %1, %14
  ret i32 %9
}
