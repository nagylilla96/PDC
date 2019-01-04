
    For($i = 32; $i -le 128; $i *= 2){
        For ($j = 1; $j -le 32; $j *= 2){
            Write-Host "Will run: size ${i}, processes ${j}"
            & mpiexec.exe -n $j .\WavesMPI.exe $i
        }
    }

