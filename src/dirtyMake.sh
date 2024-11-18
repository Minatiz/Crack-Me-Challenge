#gcc main.c crackme.o -o main
varMpiCC=$(which mpicc)


for ((i=1;i<=$#;i++)); 
do
	if [ ${!i} = "-make" ] 
	then
		$varMpiCC mainMPI.c crackme.o -o mainMPI -lm -g
		$varMpiCC mainMPIOPTIMIZED.c crackme.o -o mainMPIOPTIMIZED -lm -g

	#elif [ ${!i} = "-premake" ];
	#then 
	# 	gcc -S crackme.c -o crackme.s
	# 	gcc -c crackme.s -o crackme.o
	# 	rm crackme.s

	elif [ ${!i} = "-clean" ];
	then 
		rm mainMPI
		rm mainMPIOPTIMIZED
	fi

done;
