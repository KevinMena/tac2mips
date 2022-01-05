@staticv A0 5
@string S0 "0000\n"
@string S1 "0000Caracteres: %c %c%cNumeros: %i %i%cTextos: %s %s%c"
@string S2 "0000Hello"
@string S3 "0000 world!"
@string S4 "0000Final"
assignw NULL 0
assignw lastbase 0
assignw T0 1
@function READ 4
assignw T1 BASE[0]
read T1
return 0
@endfunction 4
@function READC 0
readc T2
return T2
@endfunction 0
@function READI 0
readi T3
return T3
@endfunction 0
@function READF 0
readf f0
return f0
@endfunction 0
@function CTOI 1
assignw T4 BASE[0]
return T4
@endfunction 1
@function ITOC 4
assignb T5 BASE[0]
return T5
@endfunction 4
@function FTOI 4
assignw f1 BASE[0]
ftoi T6 f1
return T6
@endfunction 4
@function ITOF 4
assignw T7 BASE[0]
itof f2 T7
return f2
@endfunction 4
@function PRINT 24
assignb test A0[4]
goif L0 test
assignw BASE[20] S0
@label L0
assignw T8 4
assignw T9 4
assignw T10 4
assignw T11 4
assignw T12 4
assignw T13 BASE[0]
@label L1
assignb T14 T13[T8]
add T8 T8 1
eq test T14 0
goif L1_end test
eq test T14 37
goif L2 test
printc T14
goto L1
@label L2
assignb T14 T13[T8]
add T8 T8 1
eq test T14 99
goif L3 test
eq test T14 105
goif L4 test
eq test T14 102
goif L5 test
eq test T14 115
goif L6 test
goto L1
@label L3
assignw T15 BASE[4]
assignb T16 T15[T9]
printc T16
add T9 T9 1
goto L1
@label L4
assignw T15 BASE[8]
assignw T17 T15[T10]
printi T17
add T10 T10 4
goto L1
@label L5
assignw T15 BASE[12]
assignw f3 T15[T11]
printf f3
add T11 T11 4
goto L1
@label L6
assignw T15 BASE[16]
assignw T18 T15[T12]
add T18 T18 4
print T18
add T12 T12 4
goto L1
@label L1_end
assignw T15 BASE[20]
add T15 T15 4
print T15
return 0
@endfunction 24
assignw S1[0] 50
assignb T20 65
assignb T21 9
assignb T22 10
assignb T23 10
assignb T24 10
assignw T25 5
assignw T27 T25
assignw T28 1
mult T28 T28 T27
add T28 T28 4
malloc T29 T28
assignw T29[0] T27
assignw T26 T29
assignb T26[8] T24
assignb T26[7] T23
assignb T26[6] T22
assignb T26[5] T21
assignb T26[4] T20
assignw T30 42
assignw T31 69
minus T32 T31
assignw T33 2
assignw T35 T33
assignw T36 4
mult T36 T36 T35
add T36 T36 4
malloc T37 T36
assignw T37[0] T35
assignw T34 T37
assignw T34[8] T32
assignw T34[4] T30
assignw S2[0] 5
assignw S3[0] 7
assignw T40 2
assignw T42 T40
assignw T43 4
mult T43 T43 T42
add T43 T43 4
malloc T44 T43
assignw T44[0] T42
assignw T41 T44
@label L7
sub T43 T43 4
lt test T43 4
goif L7_end test
goto L7
@label L7_end
assignw T41[8] S3
assignw T41[4] S2
assignw S4[0] 5
param T46 0
assignw T46[0] S1
assignw T48 T26[0]
assignw T49 1
mult T49 T49 T48
add T49 T49 4
malloc T50 T49
assignw T47 T50
memcpy T47 T26 T49
param T51 4
assignw T51[0] T47
assignb A0[0] 1
assignw T53 T34[0]
assignw T54 4
mult T54 T54 T53
add T54 T54 4
malloc T55 T54
assignw T52 T55
memcpy T52 T34 T54
param T56 8
assignw T56[0] T52
assignb A0[1] 1
assignb A0[2] 0
assignw T58 T41[0]
assignw T59 4
mult T59 T59 T58
add T59 T59 4
malloc T60 T59
assignw T57 T60
assignw T61 T59
@label L8
sub T61 T61 4
lt test T61 4
goif L8_end test
assignw T62 T41[T61]
assignw T63 T62[0]
assignw T64 1
mult T64 T64 T63
add T64 T64 4
malloc T65 T64
assignw T57[T61] T65
assignw T66 T57[T61]
memcpy T66 T62 T64
goto L8
@label L8_end
param T67 16
assignw T67[0] T57
assignb A0[3] 1
param T68 20
assignw T68[0] S4
assignb A0[4] 1
call T69 PRINT 6
