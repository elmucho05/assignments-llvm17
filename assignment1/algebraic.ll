; Test file created to pass functionalites
define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  ; Algebraic identity test
  ; Sum
  %3 = add nsw i32 %0, 0
  %4 = add nsw i32 0, %1
  %5 = add nsw i32 %3, 34
  %6 = add nsw i32 1, %4

  ; Sub
  %7 = sub nsw i32 %0, 0
  %8 = sub nsw i32 0, %1
  %9 = sub nsw i32 %7, 34
  %10 = sub nsw i32 1, %8

  ; Mul
  %11 = mul nsw i32 %0, 1
  %12 = mul nsw i32 1, %1
  %13 = mul nsw i32 %11, 34
  %14 = mul nsw i32 13, %12

  ; Div
  %15 = sdiv i32 %0, 1
  %16 = sdiv i32 1, %1
  %17 = sdiv i32 %15, 34
  %18 = sdiv i32 4, %16

  ret i32 %3
}
