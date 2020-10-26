#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clocale>
#include "mpi.h"

//Функция Ring: 0 --> 1, 1-->2, 2-->3 и т.д. --> 1
//И так ring раз
int main(int argc, char* argv[])
{
    //Число повторов
    int ring = 2;
    int ProcNum, ProcRank, ProcFrom, ProcTo;
    MPI_Status Status;
    char mess[] = "abcdef";
    //int mess = 5;

    //Инициализация среды
    MPI_Init(&argc, &argv);
    //Определение кол-ва процессов
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    //Определение ранга процесса
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcRank == 0)
    {
        // Действия, выполняемые только процессом с рангом 0
        MPI_Send(&mess, _countof(mess), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        printf("\n Start send:  %d -> %d", ProcRank, 1);
    }
    else // Сообщение, отправляемое всеми процессами, кроме процесса с рангом 0
    {
        for (int i = 0; i < ring; i++)
        {
            char recv[10];
            //int recv = 0;

            //Не первый круг и 1-й процесс
            if (i > 0 && ProcRank == 1) 
                ProcFrom = ProcNum - 1; //От последнего 1-му
            else 
                ProcFrom = ProcRank - 1; //От предыдущих
            MPI_Recv(&recv, _countof(recv), MPI_CHAR, ProcFrom, 0, MPI_COMM_WORLD, &Status);
            printf("\n Recv[%d]:  %d -> %d, %s", i, ProcFrom, ProcRank, recv);

            //Если последний процесс
            if (ProcRank == ProcNum - 1) 
                ProcTo = 1; //От последнего 1-му
            else 
                ProcTo = ProcRank + 1; //Иначе следующему
            MPI_Send(&recv, _countof(recv), MPI_CHAR, ProcTo, 0, MPI_COMM_WORLD);
            printf("\n Send[%d]:  %d -> %d", i, ProcRank, ProcTo);
        }
    }
    printf("\n");

    //Завершение
    MPI_Finalize(); 
    return 0;
}