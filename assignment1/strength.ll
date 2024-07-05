; Test file created to pass functionalites
define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  ; Strength reduction test
  ; Mul
  %3 = mul nsw i32 %0, 15
  %4 = mul nsw i32 13, %1
  %5 = mul nsw i32 %3, 16
  %6 = mul nsw i32 3, %4

  ; Div
  %7 = sdiv i32 %0, 15
  %8 = sdiv i32 14, %1
  %9 = sdiv i32 %7, 32
  %10 = sdiv i32 4, %8


  ret i32 %3
}
