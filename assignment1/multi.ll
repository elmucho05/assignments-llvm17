; Test file created to pass functionalites
define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  ; Multi instruction optimization test
  ; Sum
  %3 = add nsw i32 %0, 5
  %4 = sub nsw i32 %3, 5
  %5 = add nsw i32 34, %4

  %6 = add nsw i32 34, %1
  %7 = sub i32 %6, 34
  %8 = add nsw i32 0, %7

  ; Sub
  %9 = sub nsw i32 %0, 5
  %10 = add nsw i32 %9, 5
  %11 = sub nsw i32 34, %10

  ; Mul
  %12 = mul nsw i32 %0, 5
  %13 = sdiv i32 %12, 5
  %14 = add nsw i32 34, %13

  %15 = mul nsw i32 34, %1
  %16 = sdiv i32 %15, 34
  %17 = add nsw i32 0, %16

  ; Div
  %18 = sdiv i32 %0, 5
  %19 = mul nsw i32 %18, 5
  %20 = sub nsw i32 34, %19

  ret i32 %3
}
