# Statistics for grouped trace points from trace_group_id 248
# Statistics for grouped trace points from trace_group_id 248
# All data will be placed inline
# column 1: node count
# column 2: average
# column 3: std. dev.
# column 4: min()
# column 5: max()
# column 6: count()
reset
set logscale xy
set key below
set size 1, 1
set linestyle 1
set linestyle 2
set linestyle 3
set linestyle 4
set linestyle 5
set linestyle 6
set linestyle 7
set linestyle 8
set linestyle 9
set linestyle 10
set linestyle 11
set linestyle 12
set linestyle 13
set linestyle 14
set linestyle 15
set linestyle 16
set linestyle 17
set linestyle 18
set linestyle 19
set linestyle 20
set linestyle 21
set xlabel 'Node Count'
set ylabel 'Elapsed Time (nsec.)'
plot '-' using 1:2:3:4 title 'ThrobberReduce' with errorbars linestyle 1,\
'-' using 1:2:3:4 title 'P2_Simtick_Loop_Control' with errorbars linestyle 2,\
'-' using 1:2:3:4 title 'FatCompVerletListGen' with errorbars linestyle 3,\
'-' using 1:2:3:4 title 'UDF_KineticEnergy' with errorbars linestyle 4,\
'-' using 1:2:3:4 title 'MsgBcast' with errorbars linestyle 5,\
'-' using 1:2:3:4 title 'EveryNTimeSteps' with errorbars linestyle 6,\
'-' using 1:2:3:4 title 'UDF_StdHarmonicBondForce' with errorbars linestyle 7,\
'-' using 1:2:3:4 title 'ReduceForces_Cntl' with errorbars linestyle 8,\
'-' using 1:2:3:4 title 'P2_InitialBarrier' with errorbars linestyle 9,\
'-' using 1:2:3:4 title 'UDF_UpdatePosition' with errorbars linestyle 10,\
'-' using 1:2:3:4 title 'Sync' with errorbars linestyle 11,\
'-' using 1:2:3:4 title 'mFragmentMigration' with errorbars linestyle 12,\
'-' using 1:2:3:4 title 'GuardZoneViolationReduction' with errorbars linestyle 13,\
'-' using 1:2:3:4 title 'UDF_WaterTIP3PShake' with errorbars linestyle 14,\
'-' using 1:2:3:4 title 'ThrobberBcast' with errorbars linestyle 15,\
'-' using 1:2:3:4 title 'UDF_WaterTIP3PRattle' with errorbars linestyle 16,\
'-' using 1:2:3:4 title 'MsgReduce' with errorbars linestyle 17,\
'-' using 1:2:3:4 title 'P2_NSQ_Control' with errorbars linestyle 18,\
'-' using 1:2:3:4 title 'NSQ_RealSpace_Meat_' with errorbars linestyle 19,\
'-' using 1:2:3:4 title 'UDF_UpdateVelocity' with errorbars linestyle 20,\
'-' using 1:2:3:4 title 'GlobalizePositions_Cntl' with errorbars linestyle 21,\
'-' using 1:2 notitle with lines linestyle 1,\
'-' using 1:2 notitle with lines linestyle 2,\
'-' using 1:2 notitle with lines linestyle 3,\
'-' using 1:2 notitle with lines linestyle 4,\
'-' using 1:2 notitle with lines linestyle 5,\
'-' using 1:2 notitle with lines linestyle 6,\
'-' using 1:2 notitle with lines linestyle 7,\
'-' using 1:2 notitle with lines linestyle 8,\
'-' using 1:2 notitle with lines linestyle 9,\
'-' using 1:2 notitle with lines linestyle 10,\
'-' using 1:2 notitle with lines linestyle 11,\
'-' using 1:2 notitle with lines linestyle 12,\
'-' using 1:2 notitle with lines linestyle 13,\
'-' using 1:2 notitle with lines linestyle 14,\
'-' using 1:2 notitle with lines linestyle 15,\
'-' using 1:2 notitle with lines linestyle 16,\
'-' using 1:2 notitle with lines linestyle 17,\
'-' using 1:2 notitle with lines linestyle 18,\
'-' using 1:2 notitle with lines linestyle 19,\
'-' using 1:2 notitle with lines linestyle 20,\
'-' using 1:2 notitle with lines linestyle 21
512	190885.0	32908.8	129304.0
1024	156890.0	35269.9	88727.0
2048	151372.0	46336.1	57276.0
4096	128238.0	47792.5	58487.0
4096	153039.0	39639.2	45026.0
8192	109241.0	56582.7	35578.0
16384	150045.0	102940.0	12042.0
e
512	6221560.0	1207060.0	5700550.0
1024	3892640.0	607664.0	3644670.0
2048	2453310.0	398082.0	2305220.0
4096	2108680.0	288311.0	2028690.0
4096	1604900.0	321279.0	1499900.0
8192	1499140.0	315567.0	1415580.0
16384	1664040.0	616969.0	1479960.0
e
512	66226.8	427429.0	0.0
1024	34796.6	224720.0	0.0
2048	18739.4	121184.0	0.0
4096	11734.6	75929.7	0.0
4096	10850.6	70198.9	0.0
8192	6858.43	44501.8	0.0
16384	4937.84	38482.9	0.0
e
512	4593.34	593.164	2646.0
1024	3119.03	510.608	945.0
2048	2147.13	417.506	924.0
4096	1666.21	407.052	911.0
4096	1662.56	418.481	902.0
8192	1362.07	386.996	890.0
16384	1176.93	329.394	909.0
e
512	22473.9	25516.9	9217.0
1024	21876.2	32791.5	8486.0
2048	24557.2	53892.7	7246.0
4096	34783.2	101432.0	7109.0
4096	30941.6	86870.0	6760.0
8192	51655.7	180496.0	6691.0
16384	78830.5	295039.0	5977.0
e
512	98.7014	1198.37	0.0
1024	98.6398	1197.94	0.0
2048	98.9621	1202.74	0.0
4096	99.218	1204.43	0.0
4096	98.9289	1201.01	0.0
8192	102.033	1213.83	0.0
16384	102.597	1224.75	0.0
e
512	403596.0	239663.0	27265.0
1024	211026.0	136301.0	5751.0
2048	114161.0	85633.3	5705.0
4096	63898.6	56964.0	5694.0
4096	63455.5	59905.0	5700.0
8192	36300.1	41973.5	5664.0
16384	21349.0	31217.4	5394.0
e
512	1199200.0	469262.0	191878.0
1024	867628.0	308908.0	172963.0
2048	696558.0	203900.0	133210.0
4096	545190.0	145915.0	153454.0
4096	526127.0	142460.0	118479.0
8192	419513.0	128839.0	134032.0
16384	835930.0	326316.0	118392.0
e
512	0.0	0.0	0.0
1024	0.0	0.0	0.0
2048	0.0	0.0	0.0
4096	0.0	0.0	0.0
4096	0.0	0.0	0.0
8192	0.0	0.0	0.0
16384	0.0	0.0	0.0
e
512	5401.59	759.582	2963.0
1024	3405.64	570.015	779.0
2048	2350.04	517.346	767.0
4096	1746.61	534.279	761.0
4096	1742.12	548.617	763.0
8192	1345.17	524.107	757.0
16384	1083.39	437.18	744.0
e
512	0.0	0.0	0.0
1024	0.0	0.0	0.0
2048	0.0	0.0	0.0
4096	0.0	0.0	0.0
4096	0.0	0.0	0.0
8192	0.0	0.0	0.0
16384	0.0	0.0	0.0
e
512	1405.42	9026.29	0.0
1024	1486.04	9543.52	0.0
2048	1802.47	11572.1	0.0
4096	2570.77	16502.9	0.0
4096	2552.75	16387.5	0.0
8192	4106.51	26359.7	0.0
16384	7597.31	48765.4	0.0
e
512	164138.0	51700.3	13398.0
1024	110434.0	34461.3	9833.0
2048	100387.0	26857.5	8873.0
4096	98509.9	22431.1	8593.0
4096	87914.4	26352.6	7683.0
8192	78127.7	20966.0	7280.0
16384	91639.2	33584.4	7033.0
e
512	46942.5	8333.12	22266.0
1024	28415.0	6443.12	4040.0
2048	18605.0	5830.6	4023.0
4096	12670.7	5660.74	4011.0
4096	12586.5	5663.04	4017.0
8192	8870.37	5086.27	4010.0
16384	6584.78	4170.52	4023.0
e
512	162649.0	29564.0	122919.0
1024	135740.0	42780.0	98542.0
2048	122749.0	72689.2	69754.0
4096	139810.0	137563.0	71137.0
4096	122739.0	94839.2	75555.0
8192	144943.0	206351.0	55508.0
16384	188955.0	314408.0	27880.0
e
512	90160.5	45785.1	14666.0
1024	49928.2	26487.4	4501.0
2048	29246.8	17313.8	4462.0
4096	18077.1	12108.5	4439.0
4096	17984.1	12621.5	4461.0
8192	11750.5	9224.88	4450.0
16384	8258.72	7102.39	4381.0
e
512	24471.5	27105.0	4544.0
1024	23443.9	25032.1	4187.0
2048	26680.3	31517.4	4047.0
4096	22626.0	37763.4	4053.0
4096	15971.3	30417.8	4064.0
8192	17882.2	42426.5	4013.0
16384	23012.5	68554.2	4013.0
e
512	2633870.0	1270640.0	1512930.0
1024	1339210.0	661472.0	696162.0
2048	684422.0	343439.0	204564.0
4096	361829.0	191251.0	116530.0
4096	356907.0	186233.0	112949.0
8192	188433.0	107706.0	38811.0
16384	102386.0	113507.0	3085.0
e
512	2188660.0	220522.0	1509970.0
1024	1109040.0	136542.0	693189.0
2048	565801.0	80137.1	201928.0
4096	296550.0	49452.5	113092.0
4096	292844.0	48794.0	109452.0
8192	151940.0	33535.8	36161.0
16384	79725.9	69083.2	846.0
e
512	15462.2	2440.42	7531.0
1024	9015.52	1917.04	1075.0
2048	5389.61	1678.78	1068.0
4096	3484.98	1473.25	1051.0
4096	3465.91	1509.0	1053.0
8192	2384.35	1286.52	1046.0
16384	1750.35	1046.53	1042.0
e
512	271412.0	63628.0	191713.0
1024	214106.0	67013.2	150375.0
2048	179457.0	95189.6	109547.0
4096	192330.0	165639.0	115016.0
4096	169076.0	117170.0	117815.0
8192	185670.0	226496.0	92827.0
16384	225273.0	326807.0	51907.0
e
512	190885.0	32908.8	129304.0
1024	156890.0	35269.9	88727.0
2048	151372.0	46336.1	57276.0
4096	128238.0	47792.5	58487.0
4096	153039.0	39639.2	45026.0
8192	109241.0	56582.7	35578.0
16384	150045.0	102940.0	12042.0
e
512	6221560.0	1207060.0	5700550.0
1024	3892640.0	607664.0	3644670.0
2048	2453310.0	398082.0	2305220.0
4096	2108680.0	288311.0	2028690.0
4096	1604900.0	321279.0	1499900.0
8192	1499140.0	315567.0	1415580.0
16384	1664040.0	616969.0	1479960.0
e
512	66226.8	427429.0	0.0
1024	34796.6	224720.0	0.0
2048	18739.4	121184.0	0.0
4096	11734.6	75929.7	0.0
4096	10850.6	70198.9	0.0
8192	6858.43	44501.8	0.0
16384	4937.84	38482.9	0.0
e
512	4593.34	593.164	2646.0
1024	3119.03	510.608	945.0
2048	2147.13	417.506	924.0
4096	1666.21	407.052	911.0
4096	1662.56	418.481	902.0
8192	1362.07	386.996	890.0
16384	1176.93	329.394	909.0
e
512	22473.9	25516.9	9217.0
1024	21876.2	32791.5	8486.0
2048	24557.2	53892.7	7246.0
4096	34783.2	101432.0	7109.0
4096	30941.6	86870.0	6760.0
8192	51655.7	180496.0	6691.0
16384	78830.5	295039.0	5977.0
e
512	98.7014	1198.37	0.0
1024	98.6398	1197.94	0.0
2048	98.9621	1202.74	0.0
4096	99.218	1204.43	0.0
4096	98.9289	1201.01	0.0
8192	102.033	1213.83	0.0
16384	102.597	1224.75	0.0
e
512	403596.0	239663.0	27265.0
1024	211026.0	136301.0	5751.0
2048	114161.0	85633.3	5705.0
4096	63898.6	56964.0	5694.0
4096	63455.5	59905.0	5700.0
8192	36300.1	41973.5	5664.0
16384	21349.0	31217.4	5394.0
e
512	1199200.0	469262.0	191878.0
1024	867628.0	308908.0	172963.0
2048	696558.0	203900.0	133210.0
4096	545190.0	145915.0	153454.0
4096	526127.0	142460.0	118479.0
8192	419513.0	128839.0	134032.0
16384	835930.0	326316.0	118392.0
e
512	0.0	0.0	0.0
1024	0.0	0.0	0.0
2048	0.0	0.0	0.0
4096	0.0	0.0	0.0
4096	0.0	0.0	0.0
8192	0.0	0.0	0.0
16384	0.0	0.0	0.0
e
512	5401.59	759.582	2963.0
1024	3405.64	570.015	779.0
2048	2350.04	517.346	767.0
4096	1746.61	534.279	761.0
4096	1742.12	548.617	763.0
8192	1345.17	524.107	757.0
16384	1083.39	437.18	744.0
e
512	0.0	0.0	0.0
1024	0.0	0.0	0.0
2048	0.0	0.0	0.0
4096	0.0	0.0	0.0
4096	0.0	0.0	0.0
8192	0.0	0.0	0.0
16384	0.0	0.0	0.0
e
512	1405.42	9026.29	0.0
1024	1486.04	9543.52	0.0
2048	1802.47	11572.1	0.0
4096	2570.77	16502.9	0.0
4096	2552.75	16387.5	0.0
8192	4106.51	26359.7	0.0
16384	7597.31	48765.4	0.0
e
512	164138.0	51700.3	13398.0
1024	110434.0	34461.3	9833.0
2048	100387.0	26857.5	8873.0
4096	98509.9	22431.1	8593.0
4096	87914.4	26352.6	7683.0
8192	78127.7	20966.0	7280.0
16384	91639.2	33584.4	7033.0
e
512	46942.5	8333.12	22266.0
1024	28415.0	6443.12	4040.0
2048	18605.0	5830.6	4023.0
4096	12670.7	5660.74	4011.0
4096	12586.5	5663.04	4017.0
8192	8870.37	5086.27	4010.0
16384	6584.78	4170.52	4023.0
e
512	162649.0	29564.0	122919.0
1024	135740.0	42780.0	98542.0
2048	122749.0	72689.2	69754.0
4096	139810.0	137563.0	71137.0
4096	122739.0	94839.2	75555.0
8192	144943.0	206351.0	55508.0
16384	188955.0	314408.0	27880.0
e
512	90160.5	45785.1	14666.0
1024	49928.2	26487.4	4501.0
2048	29246.8	17313.8	4462.0
4096	18077.1	12108.5	4439.0
4096	17984.1	12621.5	4461.0
8192	11750.5	9224.88	4450.0
16384	8258.72	7102.39	4381.0
e
512	24471.5	27105.0	4544.0
1024	23443.9	25032.1	4187.0
2048	26680.3	31517.4	4047.0
4096	22626.0	37763.4	4053.0
4096	15971.3	30417.8	4064.0
8192	17882.2	42426.5	4013.0
16384	23012.5	68554.2	4013.0
e
512	2633870.0	1270640.0	1512930.0
1024	1339210.0	661472.0	696162.0
2048	684422.0	343439.0	204564.0
4096	361829.0	191251.0	116530.0
4096	356907.0	186233.0	112949.0
8192	188433.0	107706.0	38811.0
16384	102386.0	113507.0	3085.0
e
512	2188660.0	220522.0	1509970.0
1024	1109040.0	136542.0	693189.0
2048	565801.0	80137.1	201928.0
4096	296550.0	49452.5	113092.0
4096	292844.0	48794.0	109452.0
8192	151940.0	33535.8	36161.0
16384	79725.9	69083.2	846.0
e
512	15462.2	2440.42	7531.0
1024	9015.52	1917.04	1075.0
2048	5389.61	1678.78	1068.0
4096	3484.98	1473.25	1051.0
4096	3465.91	1509.0	1053.0
8192	2384.35	1286.52	1046.0
16384	1750.35	1046.53	1042.0
e
512	271412.0	63628.0	191713.0
1024	214106.0	67013.2	150375.0
2048	179457.0	95189.6	109547.0
4096	192330.0	165639.0	115016.0
4096	169076.0	117170.0	117815.0
8192	185670.0	226496.0	92827.0
16384	225273.0	326807.0	51907.0
e