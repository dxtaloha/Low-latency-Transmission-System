; ModuleID = 'af_xdp_kern.c'
source_filename = "af_xdp_kern.c"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "bpf"

%struct.anon = type { [2 x i32]*, i32*, i32*, [64 x i32]*, [1 x i32]* }
%struct.anon.2 = type { [17 x i32]*, i32*, i32*, [64 x i32]* }
%struct.xdp_md = type { i32, i32, i32, i32, i32, i32 }
%struct.ethhdr = type { [6 x i8], [6 x i8], i16 }

@xdp_ip_map = dso_local global %struct.anon zeroinitializer, section ".maps", align 8, !dbg !0
@xsks_map = dso_local global %struct.anon.2 zeroinitializer, section ".maps", align 8, !dbg !121
@_license = dso_local global [4 x i8] c"GPL\00", section "license", align 1, !dbg !115
@llvm.compiler.used = appending global [4 x i8*] [i8* getelementptr inbounds ([4 x i8], [4 x i8]* @_license, i32 0, i32 0), i8* bitcast (%struct.anon* @xdp_ip_map to i8*), i8* bitcast (i32 (%struct.xdp_md*)* @xdp_sock_prog to i8*), i8* bitcast (%struct.anon.2* @xsks_map to i8*)], section "llvm.metadata"

; Function Attrs: nounwind
define dso_local i32 @xdp_sock_prog(%struct.xdp_md* nocapture noundef readonly %0) #0 section "xdp" !dbg !174 {
  %2 = alloca i32, align 4
  call void @llvm.dbg.value(metadata %struct.xdp_md* %0, metadata !187, metadata !DIExpression()), !dbg !215
  %3 = getelementptr inbounds %struct.xdp_md, %struct.xdp_md* %0, i64 0, i32 4, !dbg !216
  %4 = load i32, i32* %3, align 4, !dbg !216, !tbaa !217
  call void @llvm.dbg.value(metadata i32 %4, metadata !188, metadata !DIExpression()), !dbg !215
  %5 = bitcast i32* %2 to i8*, !dbg !222
  call void @llvm.lifetime.start.p0i8(i64 4, i8* nonnull %5) #4, !dbg !222
  call void @llvm.dbg.value(metadata i32 0, metadata !189, metadata !DIExpression()), !dbg !215
  store i32 0, i32* %2, align 4, !dbg !223, !tbaa !224
  call void @llvm.dbg.value(metadata i32* %2, metadata !189, metadata !DIExpression(DW_OP_deref)), !dbg !215
  %6 = call i8* inttoptr (i64 1 to i8* (i8*, i8*)*)(i8* noundef bitcast (%struct.anon* @xdp_ip_map to i8*), i8* noundef nonnull %5) #4, !dbg !225
  %7 = bitcast i8* %6 to i32*, !dbg !225
  call void @llvm.dbg.value(metadata i32* %7, metadata !190, metadata !DIExpression()), !dbg !215
  %8 = icmp eq i8* %6, null, !dbg !226
  br i1 %8, label %73, label %9, !dbg !227

9:                                                ; preds = %1
  %10 = getelementptr inbounds %struct.xdp_md, %struct.xdp_md* %0, i64 0, i32 1, !dbg !228
  %11 = load i32, i32* %10, align 4, !dbg !228, !tbaa !229
  %12 = zext i32 %11 to i64, !dbg !230
  %13 = inttoptr i64 %12 to i8*, !dbg !231
  call void @llvm.dbg.value(metadata i8* %13, metadata !191, metadata !DIExpression()), !dbg !232
  %14 = getelementptr inbounds %struct.xdp_md, %struct.xdp_md* %0, i64 0, i32 0, !dbg !233
  %15 = load i32, i32* %14, align 4, !dbg !233, !tbaa !234
  %16 = zext i32 %15 to i64, !dbg !235
  %17 = inttoptr i64 %16 to %struct.ethhdr*, !dbg !236
  call void @llvm.dbg.value(metadata %struct.ethhdr* %17, metadata !194, metadata !DIExpression()), !dbg !232
  call void @llvm.dbg.value(metadata %struct.ethhdr* %17, metadata !195, metadata !DIExpression()), !dbg !232
  %18 = getelementptr inbounds %struct.ethhdr, %struct.ethhdr* %17, i64 1, i32 0, i64 0, !dbg !237
  %19 = icmp ugt i8* %18, %13, !dbg !239
  %20 = getelementptr inbounds %struct.ethhdr, %struct.ethhdr* %17, i64 2, i32 1, i64 0
  %21 = icmp ugt i8* %20, %13
  %22 = select i1 %19, i1 true, i1 %21, !dbg !240
  call void @llvm.dbg.value(metadata %struct.ethhdr* %17, metadata !206, metadata !DIExpression(DW_OP_plus_uconst, 14, DW_OP_stack_value)), !dbg !232
  br i1 %22, label %73, label %23, !dbg !240

23:                                               ; preds = %9
  %24 = load i8, i8* %18, align 4, !dbg !241
  %25 = and i8 %24, -16, !dbg !243
  %26 = icmp eq i8 %25, 64, !dbg !243
  br i1 %26, label %27, label %73, !dbg !244

27:                                               ; preds = %23
  %28 = shl i8 %24, 2, !dbg !245
  %29 = and i8 %28, 60, !dbg !245
  %30 = zext i8 %29 to i32, !dbg !245
  %31 = zext i8 %29 to i64
  %32 = getelementptr inbounds %struct.ethhdr, %struct.ethhdr* %17, i64 1, i32 0, i64 %31, !dbg !247
  %33 = icmp ugt i8* %32, %13, !dbg !248
  br i1 %33, label %73, label %34, !dbg !249

34:                                               ; preds = %27
  %35 = getelementptr inbounds %struct.ethhdr, %struct.ethhdr* %17, i64 1, i32 2, !dbg !250
  %36 = bitcast i16* %35 to i32*, !dbg !250
  %37 = load i32, i32* %36, align 4, !dbg !250, !tbaa !251
  %38 = load i32, i32* %7, align 4, !dbg !252, !tbaa !224
  %39 = icmp eq i32 %37, %38, !dbg !253
  br i1 %39, label %40, label %73, !dbg !254

40:                                               ; preds = %34
  %41 = getelementptr inbounds %struct.ethhdr, %struct.ethhdr* %17, i64 1, i32 1, i64 3, !dbg !255
  %42 = load i8, i8* %41, align 1, !dbg !255, !tbaa !256
  switch i8 %42, label %73 [
    i8 6, label %43
    i8 17, label %66
  ], !dbg !259

43:                                               ; preds = %40
  call void @llvm.dbg.value(metadata i8* %32, metadata !207, metadata !DIExpression()), !dbg !260
  %44 = getelementptr inbounds i8, i8* %32, i64 20, !dbg !261
  %45 = icmp ugt i8* %44, %13, !dbg !263
  br i1 %45, label %73, label %46, !dbg !264

46:                                               ; preds = %43
  call void @llvm.dbg.value(metadata i8* %32, metadata !207, metadata !DIExpression()), !dbg !260
  %47 = getelementptr inbounds i8, i8* %32, i64 12, !dbg !265
  %48 = bitcast i8* %47 to i16*, !dbg !265
  %49 = load i16, i16* %48, align 4, !dbg !265
  %50 = and i16 %49, 1792, !dbg !267
  %51 = icmp eq i16 %50, 0, !dbg !267
  br i1 %51, label %52, label %73, !dbg !267

52:                                               ; preds = %46
  %53 = and i16 %49, 4096, !dbg !268
  %54 = icmp eq i16 %53, 0, !dbg !268
  br i1 %54, label %69, label %55, !dbg !270

55:                                               ; preds = %52
  %56 = getelementptr inbounds %struct.ethhdr, %struct.ethhdr* %17, i64 1, i32 0, i64 2, !dbg !271
  %57 = bitcast i8* %56 to i16*, !dbg !271
  %58 = load i16, i16* %57, align 2, !dbg !271, !tbaa !272
  %59 = call i16 @llvm.bswap.i16(i16 %58), !dbg !271
  %60 = zext i16 %59 to i32, !dbg !271
  %61 = lshr i16 %49, 2, !dbg !273
  %62 = and i16 %61, 60, !dbg !273
  %63 = zext i16 %62 to i32, !dbg !273
  %64 = add nuw nsw i32 %63, %30, !dbg !274
  %65 = icmp eq i32 %64, %60, !dbg !275
  br i1 %65, label %73, label %69, !dbg !276

66:                                               ; preds = %40
  call void @llvm.dbg.value(metadata i8* %32, metadata !212, metadata !DIExpression()), !dbg !277
  %67 = getelementptr inbounds i8, i8* %32, i64 8, !dbg !278
  %68 = icmp ugt i8* %67, %13, !dbg !280
  br i1 %68, label %73, label %69, !dbg !281

69:                                               ; preds = %66, %52, %55
  %70 = sext i32 %4 to i64, !dbg !282
  %71 = call i64 inttoptr (i64 51 to i64 (i8*, i64, i64)*)(i8* noundef bitcast (%struct.anon.2* @xsks_map to i8*), i64 noundef %70, i64 noundef 0) #4, !dbg !282
  %72 = trunc i64 %71 to i32, !dbg !282
  br label %73, !dbg !283

73:                                               ; preds = %69, %66, %43, %46, %55, %27, %23, %9, %1, %40, %34
  %74 = phi i32 [ 2, %34 ], [ 2, %40 ], [ 2, %1 ], [ 1, %66 ], [ 2, %55 ], [ 2, %46 ], [ 1, %43 ], [ 1, %27 ], [ 2, %23 ], [ 1, %9 ], [ %72, %69 ], !dbg !215
  call void @llvm.lifetime.end.p0i8(i64 4, i8* nonnull %5) #4, !dbg !283
  ret i32 %74, !dbg !283
}

; Function Attrs: argmemonly mustprogress nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: mustprogress nofree nosync nounwind readnone speculatable willreturn
declare i16 @llvm.bswap.i16(i16) #2

; Function Attrs: argmemonly mustprogress nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #3

attributes #0 = { nounwind "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { argmemonly mustprogress nofree nosync nounwind willreturn }
attributes #2 = { mustprogress nofree nosync nounwind readnone speculatable willreturn }
attributes #3 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!169, !170, !171, !172}
!llvm.ident = !{!173}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "xdp_ip_map", scope: !2, file: !3, line: 16, type: !154, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "Ubuntu clang version 14.0.0-1ubuntu1.1", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, retainedTypes: !45, globals: !114, splitDebugInlining: false, nameTableKind: None)
!3 = !DIFile(filename: "af_xdp_kern.c", directory: "/home/pi/xdp-tutorial/advanced03-AF_XDP", checksumkind: CSK_MD5, checksum: "9329e3e5922ac668c5d50675848e736e")
!4 = !{!5, !14}
!5 = !DICompositeType(tag: DW_TAG_enumeration_type, name: "xdp_action", file: !6, line: 5450, baseType: !7, size: 32, elements: !8)
!6 = !DIFile(filename: "/usr/include/linux/bpf.h", directory: "", checksumkind: CSK_MD5, checksum: "fe486ce1b008b02b4869d1c3953168cc")
!7 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!8 = !{!9, !10, !11, !12, !13}
!9 = !DIEnumerator(name: "XDP_ABORTED", value: 0)
!10 = !DIEnumerator(name: "XDP_DROP", value: 1)
!11 = !DIEnumerator(name: "XDP_PASS", value: 2)
!12 = !DIEnumerator(name: "XDP_TX", value: 3)
!13 = !DIEnumerator(name: "XDP_REDIRECT", value: 4)
!14 = !DICompositeType(tag: DW_TAG_enumeration_type, file: !15, line: 28, baseType: !7, size: 32, elements: !16)
!15 = !DIFile(filename: "/usr/include/linux/in.h", directory: "", checksumkind: CSK_MD5, checksum: "078a32220dc819f6a7e2ea3cecc4e133")
!16 = !{!17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44}
!17 = !DIEnumerator(name: "IPPROTO_IP", value: 0)
!18 = !DIEnumerator(name: "IPPROTO_ICMP", value: 1)
!19 = !DIEnumerator(name: "IPPROTO_IGMP", value: 2)
!20 = !DIEnumerator(name: "IPPROTO_IPIP", value: 4)
!21 = !DIEnumerator(name: "IPPROTO_TCP", value: 6)
!22 = !DIEnumerator(name: "IPPROTO_EGP", value: 8)
!23 = !DIEnumerator(name: "IPPROTO_PUP", value: 12)
!24 = !DIEnumerator(name: "IPPROTO_UDP", value: 17)
!25 = !DIEnumerator(name: "IPPROTO_IDP", value: 22)
!26 = !DIEnumerator(name: "IPPROTO_TP", value: 29)
!27 = !DIEnumerator(name: "IPPROTO_DCCP", value: 33)
!28 = !DIEnumerator(name: "IPPROTO_IPV6", value: 41)
!29 = !DIEnumerator(name: "IPPROTO_RSVP", value: 46)
!30 = !DIEnumerator(name: "IPPROTO_GRE", value: 47)
!31 = !DIEnumerator(name: "IPPROTO_ESP", value: 50)
!32 = !DIEnumerator(name: "IPPROTO_AH", value: 51)
!33 = !DIEnumerator(name: "IPPROTO_MTP", value: 92)
!34 = !DIEnumerator(name: "IPPROTO_BEETPH", value: 94)
!35 = !DIEnumerator(name: "IPPROTO_ENCAP", value: 98)
!36 = !DIEnumerator(name: "IPPROTO_PIM", value: 103)
!37 = !DIEnumerator(name: "IPPROTO_COMP", value: 108)
!38 = !DIEnumerator(name: "IPPROTO_SCTP", value: 132)
!39 = !DIEnumerator(name: "IPPROTO_UDPLITE", value: 136)
!40 = !DIEnumerator(name: "IPPROTO_MPLS", value: 137)
!41 = !DIEnumerator(name: "IPPROTO_ETHERNET", value: 143)
!42 = !DIEnumerator(name: "IPPROTO_RAW", value: 255)
!43 = !DIEnumerator(name: "IPPROTO_MPTCP", value: 262)
!44 = !DIEnumerator(name: "IPPROTO_MAX", value: 263)
!45 = !{!46, !47, !48, !84, !85, !106}
!46 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!47 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
!48 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !49, size: 64)
!49 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "iphdr", file: !50, line: 87, size: 160, elements: !51)
!50 = !DIFile(filename: "/usr/include/linux/ip.h", directory: "", checksumkind: CSK_MD5, checksum: "042b09a58768855e3578a0a8eba49be7")
!51 = !{!52, !56, !57, !58, !63, !64, !65, !66, !67, !69}
!52 = !DIDerivedType(tag: DW_TAG_member, name: "ihl", scope: !49, file: !50, line: 89, baseType: !53, size: 4, flags: DIFlagBitField, extraData: i64 0)
!53 = !DIDerivedType(tag: DW_TAG_typedef, name: "__u8", file: !54, line: 21, baseType: !55)
!54 = !DIFile(filename: "/usr/include/asm-generic/int-ll64.h", directory: "", checksumkind: CSK_MD5, checksum: "b810f270733e106319b67ef512c6246e")
!55 = !DIBasicType(name: "unsigned char", size: 8, encoding: DW_ATE_unsigned_char)
!56 = !DIDerivedType(tag: DW_TAG_member, name: "version", scope: !49, file: !50, line: 90, baseType: !53, size: 4, offset: 4, flags: DIFlagBitField, extraData: i64 0)
!57 = !DIDerivedType(tag: DW_TAG_member, name: "tos", scope: !49, file: !50, line: 97, baseType: !53, size: 8, offset: 8)
!58 = !DIDerivedType(tag: DW_TAG_member, name: "tot_len", scope: !49, file: !50, line: 98, baseType: !59, size: 16, offset: 16)
!59 = !DIDerivedType(tag: DW_TAG_typedef, name: "__be16", file: !60, line: 25, baseType: !61)
!60 = !DIFile(filename: "/usr/include/linux/types.h", directory: "", checksumkind: CSK_MD5, checksum: "52ec79a38e49ac7d1dc9e146ba88a7b1")
!61 = !DIDerivedType(tag: DW_TAG_typedef, name: "__u16", file: !54, line: 24, baseType: !62)
!62 = !DIBasicType(name: "unsigned short", size: 16, encoding: DW_ATE_unsigned)
!63 = !DIDerivedType(tag: DW_TAG_member, name: "id", scope: !49, file: !50, line: 99, baseType: !59, size: 16, offset: 32)
!64 = !DIDerivedType(tag: DW_TAG_member, name: "frag_off", scope: !49, file: !50, line: 100, baseType: !59, size: 16, offset: 48)
!65 = !DIDerivedType(tag: DW_TAG_member, name: "ttl", scope: !49, file: !50, line: 101, baseType: !53, size: 8, offset: 64)
!66 = !DIDerivedType(tag: DW_TAG_member, name: "protocol", scope: !49, file: !50, line: 102, baseType: !53, size: 8, offset: 72)
!67 = !DIDerivedType(tag: DW_TAG_member, name: "check", scope: !49, file: !50, line: 103, baseType: !68, size: 16, offset: 80)
!68 = !DIDerivedType(tag: DW_TAG_typedef, name: "__sum16", file: !60, line: 31, baseType: !61)
!69 = !DIDerivedType(tag: DW_TAG_member, scope: !49, file: !50, line: 104, baseType: !70, size: 64, offset: 96)
!70 = distinct !DICompositeType(tag: DW_TAG_union_type, scope: !49, file: !50, line: 104, size: 64, elements: !71)
!71 = !{!72, !79}
!72 = !DIDerivedType(tag: DW_TAG_member, scope: !70, file: !50, line: 104, baseType: !73, size: 64)
!73 = distinct !DICompositeType(tag: DW_TAG_structure_type, scope: !70, file: !50, line: 104, size: 64, elements: !74)
!74 = !{!75, !78}
!75 = !DIDerivedType(tag: DW_TAG_member, name: "saddr", scope: !73, file: !50, line: 104, baseType: !76, size: 32)
!76 = !DIDerivedType(tag: DW_TAG_typedef, name: "__be32", file: !60, line: 27, baseType: !77)
!77 = !DIDerivedType(tag: DW_TAG_typedef, name: "__u32", file: !54, line: 27, baseType: !7)
!78 = !DIDerivedType(tag: DW_TAG_member, name: "daddr", scope: !73, file: !50, line: 104, baseType: !76, size: 32, offset: 32)
!79 = !DIDerivedType(tag: DW_TAG_member, name: "addrs", scope: !70, file: !50, line: 104, baseType: !80, size: 64)
!80 = distinct !DICompositeType(tag: DW_TAG_structure_type, scope: !70, file: !50, line: 104, size: 64, elements: !81)
!81 = !{!82, !83}
!82 = !DIDerivedType(tag: DW_TAG_member, name: "saddr", scope: !80, file: !50, line: 104, baseType: !76, size: 32)
!83 = !DIDerivedType(tag: DW_TAG_member, name: "daddr", scope: !80, file: !50, line: 104, baseType: !76, size: 32, offset: 32)
!84 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !53, size: 64)
!85 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !86, size: 64)
!86 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "tcphdr", file: !87, line: 25, size: 160, elements: !88)
!87 = !DIFile(filename: "/usr/include/linux/tcp.h", directory: "", checksumkind: CSK_MD5, checksum: "8d74bf2133e7b3dab885994b9916aa13")
!88 = !{!89, !90, !91, !92, !93, !94, !95, !96, !97, !98, !99, !100, !101, !102, !103, !104, !105}
!89 = !DIDerivedType(tag: DW_TAG_member, name: "source", scope: !86, file: !87, line: 26, baseType: !59, size: 16)
!90 = !DIDerivedType(tag: DW_TAG_member, name: "dest", scope: !86, file: !87, line: 27, baseType: !59, size: 16, offset: 16)
!91 = !DIDerivedType(tag: DW_TAG_member, name: "seq", scope: !86, file: !87, line: 28, baseType: !76, size: 32, offset: 32)
!92 = !DIDerivedType(tag: DW_TAG_member, name: "ack_seq", scope: !86, file: !87, line: 29, baseType: !76, size: 32, offset: 64)
!93 = !DIDerivedType(tag: DW_TAG_member, name: "res1", scope: !86, file: !87, line: 31, baseType: !61, size: 4, offset: 96, flags: DIFlagBitField, extraData: i64 96)
!94 = !DIDerivedType(tag: DW_TAG_member, name: "doff", scope: !86, file: !87, line: 32, baseType: !61, size: 4, offset: 100, flags: DIFlagBitField, extraData: i64 96)
!95 = !DIDerivedType(tag: DW_TAG_member, name: "fin", scope: !86, file: !87, line: 33, baseType: !61, size: 1, offset: 104, flags: DIFlagBitField, extraData: i64 96)
!96 = !DIDerivedType(tag: DW_TAG_member, name: "syn", scope: !86, file: !87, line: 34, baseType: !61, size: 1, offset: 105, flags: DIFlagBitField, extraData: i64 96)
!97 = !DIDerivedType(tag: DW_TAG_member, name: "rst", scope: !86, file: !87, line: 35, baseType: !61, size: 1, offset: 106, flags: DIFlagBitField, extraData: i64 96)
!98 = !DIDerivedType(tag: DW_TAG_member, name: "psh", scope: !86, file: !87, line: 36, baseType: !61, size: 1, offset: 107, flags: DIFlagBitField, extraData: i64 96)
!99 = !DIDerivedType(tag: DW_TAG_member, name: "ack", scope: !86, file: !87, line: 37, baseType: !61, size: 1, offset: 108, flags: DIFlagBitField, extraData: i64 96)
!100 = !DIDerivedType(tag: DW_TAG_member, name: "urg", scope: !86, file: !87, line: 38, baseType: !61, size: 1, offset: 109, flags: DIFlagBitField, extraData: i64 96)
!101 = !DIDerivedType(tag: DW_TAG_member, name: "ece", scope: !86, file: !87, line: 39, baseType: !61, size: 1, offset: 110, flags: DIFlagBitField, extraData: i64 96)
!102 = !DIDerivedType(tag: DW_TAG_member, name: "cwr", scope: !86, file: !87, line: 40, baseType: !61, size: 1, offset: 111, flags: DIFlagBitField, extraData: i64 96)
!103 = !DIDerivedType(tag: DW_TAG_member, name: "window", scope: !86, file: !87, line: 55, baseType: !59, size: 16, offset: 112)
!104 = !DIDerivedType(tag: DW_TAG_member, name: "check", scope: !86, file: !87, line: 56, baseType: !68, size: 16, offset: 128)
!105 = !DIDerivedType(tag: DW_TAG_member, name: "urg_ptr", scope: !86, file: !87, line: 57, baseType: !59, size: 16, offset: 144)
!106 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !107, size: 64)
!107 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "udphdr", file: !108, line: 23, size: 64, elements: !109)
!108 = !DIFile(filename: "/usr/include/linux/udp.h", directory: "", checksumkind: CSK_MD5, checksum: "53c0d42e1bf6d93b39151764be2d20fb")
!109 = !{!110, !111, !112, !113}
!110 = !DIDerivedType(tag: DW_TAG_member, name: "source", scope: !107, file: !108, line: 24, baseType: !59, size: 16)
!111 = !DIDerivedType(tag: DW_TAG_member, name: "dest", scope: !107, file: !108, line: 25, baseType: !59, size: 16, offset: 16)
!112 = !DIDerivedType(tag: DW_TAG_member, name: "len", scope: !107, file: !108, line: 26, baseType: !59, size: 16, offset: 32)
!113 = !DIDerivedType(tag: DW_TAG_member, name: "check", scope: !107, file: !108, line: 27, baseType: !68, size: 16, offset: 48)
!114 = !{!115, !0, !121, !139, !147}
!115 = !DIGlobalVariableExpression(var: !116, expr: !DIExpression())
!116 = distinct !DIGlobalVariable(name: "_license", scope: !2, file: !3, line: 103, type: !117, isLocal: false, isDefinition: true)
!117 = !DICompositeType(tag: DW_TAG_array_type, baseType: !118, size: 32, elements: !119)
!118 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!119 = !{!120}
!120 = !DISubrange(count: 4)
!121 = !DIGlobalVariableExpression(var: !122, expr: !DIExpression())
!122 = distinct !DIGlobalVariable(name: "xsks_map", scope: !2, file: !3, line: 23, type: !123, isLocal: false, isDefinition: true)
!123 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !3, line: 18, size: 256, elements: !124)
!124 = !{!125, !131, !133, !134}
!125 = !DIDerivedType(tag: DW_TAG_member, name: "type", scope: !123, file: !3, line: 19, baseType: !126, size: 64)
!126 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !127, size: 64)
!127 = !DICompositeType(tag: DW_TAG_array_type, baseType: !128, size: 544, elements: !129)
!128 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!129 = !{!130}
!130 = !DISubrange(count: 17)
!131 = !DIDerivedType(tag: DW_TAG_member, name: "key", scope: !123, file: !3, line: 20, baseType: !132, size: 64, offset: 64)
!132 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !77, size: 64)
!133 = !DIDerivedType(tag: DW_TAG_member, name: "value", scope: !123, file: !3, line: 21, baseType: !132, size: 64, offset: 128)
!134 = !DIDerivedType(tag: DW_TAG_member, name: "max_entries", scope: !123, file: !3, line: 22, baseType: !135, size: 64, offset: 192)
!135 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !136, size: 64)
!136 = !DICompositeType(tag: DW_TAG_array_type, baseType: !128, size: 2048, elements: !137)
!137 = !{!138}
!138 = !DISubrange(count: 64)
!139 = !DIGlobalVariableExpression(var: !140, expr: !DIExpression())
!140 = distinct !DIGlobalVariable(name: "bpf_map_lookup_elem", scope: !2, file: !141, line: 56, type: !142, isLocal: true, isDefinition: true)
!141 = !DIFile(filename: "../lib/install/include/bpf/bpf_helper_defs.h", directory: "/home/pi/xdp-tutorial/advanced03-AF_XDP", checksumkind: CSK_MD5, checksum: "7422ca06c9dc86eba2f268a57d8acf2f")
!142 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !143, size: 64)
!143 = !DISubroutineType(types: !144)
!144 = !{!46, !46, !145}
!145 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !146, size: 64)
!146 = !DIDerivedType(tag: DW_TAG_const_type, baseType: null)
!147 = !DIGlobalVariableExpression(var: !148, expr: !DIExpression())
!148 = distinct !DIGlobalVariable(name: "bpf_redirect_map", scope: !2, file: !141, line: 1323, type: !149, isLocal: true, isDefinition: true)
!149 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !150, size: 64)
!150 = !DISubroutineType(types: !151)
!151 = !{!47, !46, !152, !152}
!152 = !DIDerivedType(tag: DW_TAG_typedef, name: "__u64", file: !54, line: 31, baseType: !153)
!153 = !DIBasicType(name: "unsigned long long", size: 64, encoding: DW_ATE_unsigned)
!154 = distinct !DICompositeType(tag: DW_TAG_structure_type, file: !3, line: 10, size: 320, elements: !155)
!155 = !{!156, !161, !162, !163, !164}
!156 = !DIDerivedType(tag: DW_TAG_member, name: "type", scope: !154, file: !3, line: 11, baseType: !157, size: 64)
!157 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !158, size: 64)
!158 = !DICompositeType(tag: DW_TAG_array_type, baseType: !128, size: 64, elements: !159)
!159 = !{!160}
!160 = !DISubrange(count: 2)
!161 = !DIDerivedType(tag: DW_TAG_member, name: "key", scope: !154, file: !3, line: 12, baseType: !132, size: 64, offset: 64)
!162 = !DIDerivedType(tag: DW_TAG_member, name: "value", scope: !154, file: !3, line: 13, baseType: !132, size: 64, offset: 128)
!163 = !DIDerivedType(tag: DW_TAG_member, name: "max_entries", scope: !154, file: !3, line: 14, baseType: !135, size: 64, offset: 192)
!164 = !DIDerivedType(tag: DW_TAG_member, name: "pinning", scope: !154, file: !3, line: 15, baseType: !165, size: 64, offset: 256)
!165 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !166, size: 64)
!166 = !DICompositeType(tag: DW_TAG_array_type, baseType: !128, size: 32, elements: !167)
!167 = !{!168}
!168 = !DISubrange(count: 1)
!169 = !{i32 7, !"Dwarf Version", i32 5}
!170 = !{i32 2, !"Debug Info Version", i32 3}
!171 = !{i32 1, !"wchar_size", i32 4}
!172 = !{i32 7, !"frame-pointer", i32 2}
!173 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
!174 = distinct !DISubprogram(name: "xdp_sock_prog", scope: !3, file: !3, line: 32, type: !175, scopeLine: 32, flags: DIFlagPrototyped | DIFlagAllCallsDescribed, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !2, retainedNodes: !186)
!175 = !DISubroutineType(types: !176)
!176 = !{!128, !177}
!177 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !178, size: 64)
!178 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "xdp_md", file: !6, line: 5461, size: 192, elements: !179)
!179 = !{!180, !181, !182, !183, !184, !185}
!180 = !DIDerivedType(tag: DW_TAG_member, name: "data", scope: !178, file: !6, line: 5462, baseType: !77, size: 32)
!181 = !DIDerivedType(tag: DW_TAG_member, name: "data_end", scope: !178, file: !6, line: 5463, baseType: !77, size: 32, offset: 32)
!182 = !DIDerivedType(tag: DW_TAG_member, name: "data_meta", scope: !178, file: !6, line: 5464, baseType: !77, size: 32, offset: 64)
!183 = !DIDerivedType(tag: DW_TAG_member, name: "ingress_ifindex", scope: !178, file: !6, line: 5466, baseType: !77, size: 32, offset: 96)
!184 = !DIDerivedType(tag: DW_TAG_member, name: "rx_queue_index", scope: !178, file: !6, line: 5467, baseType: !77, size: 32, offset: 128)
!185 = !DIDerivedType(tag: DW_TAG_member, name: "egress_ifindex", scope: !178, file: !6, line: 5469, baseType: !77, size: 32, offset: 160)
!186 = !{!187, !188, !189, !190, !191, !194, !195, !206, !207, !212}
!187 = !DILocalVariable(name: "ctx", arg: 1, scope: !174, file: !3, line: 32, type: !177)
!188 = !DILocalVariable(name: "index", scope: !174, file: !3, line: 33, type: !128)
!189 = !DILocalVariable(name: "key", scope: !174, file: !3, line: 34, type: !77)
!190 = !DILocalVariable(name: "ip_addr", scope: !174, file: !3, line: 35, type: !132)
!191 = !DILocalVariable(name: "data_end", scope: !192, file: !3, line: 39, type: !46)
!192 = distinct !DILexicalBlock(scope: !193, file: !3, line: 38, column: 18)
!193 = distinct !DILexicalBlock(scope: !174, file: !3, line: 38, column: 9)
!194 = !DILocalVariable(name: "data", scope: !192, file: !3, line: 40, type: !46)
!195 = !DILocalVariable(name: "eth", scope: !192, file: !3, line: 41, type: !196)
!196 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !197, size: 64)
!197 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "ethhdr", file: !198, line: 168, size: 112, elements: !199)
!198 = !DIFile(filename: "/usr/include/linux/if_ether.h", directory: "", checksumkind: CSK_MD5, checksum: "ab0320da726e75d904811ce344979934")
!199 = !{!200, !204, !205}
!200 = !DIDerivedType(tag: DW_TAG_member, name: "h_dest", scope: !197, file: !198, line: 169, baseType: !201, size: 48)
!201 = !DICompositeType(tag: DW_TAG_array_type, baseType: !55, size: 48, elements: !202)
!202 = !{!203}
!203 = !DISubrange(count: 6)
!204 = !DIDerivedType(tag: DW_TAG_member, name: "h_source", scope: !197, file: !198, line: 170, baseType: !201, size: 48, offset: 48)
!205 = !DIDerivedType(tag: DW_TAG_member, name: "h_proto", scope: !197, file: !198, line: 171, baseType: !59, size: 16, offset: 96)
!206 = !DILocalVariable(name: "ip", scope: !192, file: !3, line: 48, type: !48)
!207 = !DILocalVariable(name: "tcp", scope: !208, file: !3, line: 67, type: !85)
!208 = distinct !DILexicalBlock(scope: !209, file: !3, line: 66, column: 46)
!209 = distinct !DILexicalBlock(scope: !210, file: !3, line: 66, column: 17)
!210 = distinct !DILexicalBlock(scope: !211, file: !3, line: 65, column: 36)
!211 = distinct !DILexicalBlock(scope: !192, file: !3, line: 65, column: 13)
!212 = !DILocalVariable(name: "udp", scope: !213, file: !3, line: 88, type: !106)
!213 = distinct !DILexicalBlock(scope: !214, file: !3, line: 87, column: 53)
!214 = distinct !DILexicalBlock(scope: !209, file: !3, line: 87, column: 24)
!215 = !DILocation(line: 0, scope: !174)
!216 = !DILocation(line: 33, column: 22, scope: !174)
!217 = !{!218, !219, i64 16}
!218 = !{!"xdp_md", !219, i64 0, !219, i64 4, !219, i64 8, !219, i64 12, !219, i64 16, !219, i64 20}
!219 = !{!"int", !220, i64 0}
!220 = !{!"omnipotent char", !221, i64 0}
!221 = !{!"Simple C/C++ TBAA"}
!222 = !DILocation(line: 34, column: 5, scope: !174)
!223 = !DILocation(line: 34, column: 11, scope: !174)
!224 = !{!219, !219, i64 0}
!225 = !DILocation(line: 37, column: 15, scope: !174)
!226 = !DILocation(line: 38, column: 9, scope: !193)
!227 = !DILocation(line: 38, column: 9, scope: !174)
!228 = !DILocation(line: 39, column: 45, scope: !192)
!229 = !{!218, !219, i64 4}
!230 = !DILocation(line: 39, column: 34, scope: !192)
!231 = !DILocation(line: 39, column: 26, scope: !192)
!232 = !DILocation(line: 0, scope: !192)
!233 = !DILocation(line: 40, column: 41, scope: !192)
!234 = !{!218, !219, i64 0}
!235 = !DILocation(line: 40, column: 30, scope: !192)
!236 = !DILocation(line: 41, column: 30, scope: !192)
!237 = !DILocation(line: 44, column: 13, scope: !238)
!238 = distinct !DILexicalBlock(scope: !192, file: !3, line: 44, column: 13)
!239 = !DILocation(line: 44, column: 31, scope: !238)
!240 = !DILocation(line: 44, column: 13, scope: !192)
!241 = !DILocation(line: 56, column: 17, scope: !242)
!242 = distinct !DILexicalBlock(scope: !192, file: !3, line: 56, column: 13)
!243 = !DILocation(line: 56, column: 25, scope: !242)
!244 = !DILocation(line: 56, column: 13, scope: !192)
!245 = !DILocation(line: 61, column: 43, scope: !246)
!246 = distinct !DILexicalBlock(scope: !192, file: !3, line: 61, column: 13)
!247 = !DILocation(line: 61, column: 33, scope: !246)
!248 = !DILocation(line: 61, column: 48, scope: !246)
!249 = !DILocation(line: 61, column: 13, scope: !192)
!250 = !DILocation(line: 65, column: 17, scope: !211)
!251 = !{!220, !220, i64 0}
!252 = !DILocation(line: 65, column: 26, scope: !211)
!253 = !DILocation(line: 65, column: 23, scope: !211)
!254 = !DILocation(line: 65, column: 13, scope: !192)
!255 = !DILocation(line: 66, column: 21, scope: !209)
!256 = !{!257, !220, i64 9}
!257 = !{!"iphdr", !220, i64 0, !220, i64 0, !220, i64 1, !258, i64 2, !258, i64 4, !258, i64 6, !220, i64 8, !220, i64 9, !258, i64 10, !220, i64 12}
!258 = !{!"short", !220, i64 0}
!259 = !DILocation(line: 66, column: 17, scope: !210)
!260 = !DILocation(line: 0, scope: !208)
!261 = !DILocation(line: 70, column: 34, scope: !262)
!262 = distinct !DILexicalBlock(scope: !208, file: !3, line: 70, column: 21)
!263 = !DILocation(line: 70, column: 39, scope: !262)
!264 = !DILocation(line: 70, column: 21, scope: !208)
!265 = !DILocation(line: 75, column: 26, scope: !266)
!266 = distinct !DILexicalBlock(scope: !208, file: !3, line: 75, column: 21)
!267 = !DILocation(line: 75, column: 30, scope: !266)
!268 = !DILocation(line: 80, column: 21, scope: !269)
!269 = distinct !DILexicalBlock(scope: !208, file: !3, line: 80, column: 21)
!270 = !DILocation(line: 80, column: 30, scope: !269)
!271 = !DILocation(line: 80, column: 34, scope: !269)
!272 = !{!257, !258, i64 2}
!273 = !DILocation(line: 80, column: 83, scope: !269)
!274 = !DILocation(line: 80, column: 71, scope: !269)
!275 = !DILocation(line: 80, column: 87, scope: !269)
!276 = !DILocation(line: 80, column: 21, scope: !208)
!277 = !DILocation(line: 0, scope: !213)
!278 = !DILocation(line: 91, column: 34, scope: !279)
!279 = distinct !DILexicalBlock(scope: !213, file: !3, line: 91, column: 21)
!280 = !DILocation(line: 91, column: 39, scope: !279)
!281 = !DILocation(line: 91, column: 21, scope: !213)
!282 = !DILocation(line: 0, scope: !209)
!283 = !DILocation(line: 101, column: 1, scope: !174)
