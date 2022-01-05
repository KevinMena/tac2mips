@staticv A0 5
@string S0 "0000\n"
@string S1 "0000Input: "
@string S2 "0000Hasta luego!"
@string S3 "0000Resultado calculado!"
@string S4 "0000%i"
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
@function Function0 4
@label L7
assignw T19 0
assignw T20 BASE[0]
eq test T20 T19
goif B108 test
goto B112
@label B108
assignw T21 1
assignw lastbase BASE
return T21
goto Function0_end
@label B112
assignw T22 1
assignw T24 BASE[0]
sub T23 T24 T22
param T25 0
assignw T25[0] T23
call T26 Function0 1
assignw T28 BASE[0]
mult T27 T28 T26
assignw lastbase BASE
return T27
@label Function0_end
assignw lastbase BASE
return 0
@endfunction 4
assignw S1[0] 7
param T31 0
assignw T31[0] S1
assignb A0[0] 0
assignb A0[1] 0
assignb A0[2] 0
assignb A0[3] 0
assignb A0[4] 0
call T32 PRINT 6
call T33 READI 0
assignw T29 T33
assignw T34 0
lt test T29 T34
goif B141 test
goto B153
@label B141
assignw S2[0] 12
param T36 0
assignw T36[0] S2
assignb A0[0] 0
assignb A0[1] 0
assignb A0[2] 0
assignb A0[3] 0
assignb A0[4] 0
call T37 PRINT 6
assignw T38 0
exit T38
goto B153
@label B153
assignw S3[0] 20
param T40 0
assignw T40[0] S3
assignb A0[0] 0
assignb A0[1] 0
assignb A0[2] 0
assignb A0[3] 0
assignb A0[4] 0
call T41 PRINT 6
param T42 0
assignw T42[0] T29
call T43 Function0 1
assignw T44 T43
assignw S4[0] 2
assignw T46 1
assignw T48 T46
assignw T49 4
mult T49 T49 T48
add T49 T49 4
malloc T50 T49
assignw T50[0] T48
assignw T47 T50
assignw T47[4] T44
param T51 0
assignw T51[0] S4
assignb A0[0] 0
assignw T53 T47[0]
assignw T54 4
mult T54 T54 T53
add T54 T54 4
malloc T55 T54
assignw T52 T55
memcpy T52 T47 T54
param T56 8
assignw T56[0] T52
assignb A0[1] 1
assignb A0[2] 0
assignb A0[3] 0
assignb A0[4] 0
call T57 PRINT 6
assignw T58 0
exit T58
