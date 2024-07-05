; ModuleID = 'Foo.bc'
source_filename = "LoopUnguarded.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @foo(i32 noundef %0, ptr noundef %1, ptr noundef %2, ptr noundef %3, ptr noundef %4, i32 noundef %5) #0 !dbg !9 {
  br label %7, !dbg !12

7:                                                ; preds = %20, %6
  %.0 = phi i32 [ 0, %6 ], [ %21, %20 ], !dbg !13
  %8 = icmp slt i32 %.0, %0, !dbg !14
  br i1 %8, label %9, label %22, !dbg !15

9:                                                ; preds = %7
  %10 = sext i32 %.0 to i64, !dbg !16
  %11 = getelementptr inbounds i32, ptr %2, i64 %10, !dbg !16
  %12 = load i32, ptr %11, align 4, !dbg !16
  %13 = sdiv i32 1, %12, !dbg !17
  %14 = sext i32 %.0 to i64, !dbg !18
  %15 = getelementptr inbounds i32, ptr %3, i64 %14, !dbg !18
  %16 = load i32, ptr %15, align 4, !dbg !18
  %17 = mul nsw i32 %13, %16, !dbg !19
  %18 = sext i32 %.0 to i64, !dbg !20
  %19 = getelementptr inbounds i32, ptr %1, i64 %18, !dbg !20
  store i32 %17, ptr %19, align 4, !dbg !21
  br label %20, !dbg !22

20:                                               ; preds = %9
  %21 = add nsw i32 %.0, 1, !dbg !23
  br label %7, !dbg !15, !llvm.loop !24

22:                                               ; preds = %7
  br label %23, !dbg !26

23:                                               ; preds = %35, %22
  %.1 = phi i32 [ 0, %22 ], [ %36, %35 ], !dbg !27
  %24 = icmp slt i32 %.1, %0, !dbg !28
  br i1 %24, label %25, label %37, !dbg !29

25:                                               ; preds = %23
  %26 = sext i32 %.1 to i64, !dbg !30
  %27 = getelementptr inbounds i32, ptr %1, i64 %26, !dbg !30
  %28 = load i32, ptr %27, align 4, !dbg !30
  %29 = sext i32 %.1 to i64, !dbg !31
  %30 = getelementptr inbounds i32, ptr %3, i64 %29, !dbg !31
  %31 = load i32, ptr %30, align 4, !dbg !31
  %32 = add nsw i32 %28, %31, !dbg !32
  %33 = sext i32 %.1 to i64, !dbg !33
  %34 = getelementptr inbounds i32, ptr %4, i64 %33, !dbg !33
  store i32 %32, ptr %34, align 4, !dbg !34
  br label %35, !dbg !35

35:                                               ; preds = %25
  %36 = add nsw i32 %.1, 1, !dbg !36
  br label %23, !dbg !29, !llvm.loop !37

37:                                               ; preds = %23
  ret i32 0, !dbg !38
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7}
!llvm.ident = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "clang version 17.0.6", isOptimized: false, runtimeVersion: 0, emissionKind: NoDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "LoopUnguarded.c", directory: "/home/gerti/Desktop/LLVM17/TEST/quarto")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 8, !"PIC Level", i32 2}
!5 = !{i32 7, !"PIE Level", i32 2}
!6 = !{i32 7, !"uwtable", i32 2}
!7 = !{i32 7, !"frame-pointer", i32 2}
!8 = !{!"clang version 17.0.6"}
!9 = distinct !DISubprogram(name: "foo", scope: !1, file: !1, line: 3, type: !10, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)
!10 = !DISubroutineType(types: !11)
!11 = !{}
!12 = !DILocation(line: 4, column: 10, scope: !9)
!13 = !DILocation(line: 4, scope: !9)
!14 = !DILocation(line: 4, column: 16, scope: !9)
!15 = !DILocation(line: 4, column: 5, scope: !9)
!16 = !DILocation(line: 5, column: 17, scope: !9)
!17 = !DILocation(line: 5, column: 16, scope: !9)
!18 = !DILocation(line: 5, column: 22, scope: !9)
!19 = !DILocation(line: 5, column: 21, scope: !9)
!20 = !DILocation(line: 5, column: 9, scope: !9)
!21 = !DILocation(line: 5, column: 13, scope: !9)
!22 = !DILocation(line: 6, column: 5, scope: !9)
!23 = !DILocation(line: 4, column: 21, scope: !9)
!24 = distinct !{!24, !15, !22, !25}
!25 = !{!"llvm.loop.mustprogress"}
!26 = !DILocation(line: 7, column: 10, scope: !9)
!27 = !DILocation(line: 7, scope: !9)
!28 = !DILocation(line: 7, column: 16, scope: !9)
!29 = !DILocation(line: 7, column: 5, scope: !9)
!30 = !DILocation(line: 8, column: 16, scope: !9)
!31 = !DILocation(line: 8, column: 21, scope: !9)
!32 = !DILocation(line: 8, column: 20, scope: !9)
!33 = !DILocation(line: 8, column: 9, scope: !9)
!34 = !DILocation(line: 8, column: 14, scope: !9)
!35 = !DILocation(line: 9, column: 5, scope: !9)
!36 = !DILocation(line: 7, column: 21, scope: !9)
!37 = distinct !{!37, !29, !35, !25}
!38 = !DILocation(line: 10, column: 5, scope: !9)
