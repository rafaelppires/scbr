#!/bin/bash
set -e

Suffix=IN-PLAIN
OutputDir="/home/rafaelpp/dev/pubsub-corp-poc/results/final-results/"
WorkloadsDir="/home/rafaelpp/dev/workloads/"
Execname="./enclave_CBR_Enclave_Filter/sample"

function executewload {
    echo $1
    CurDir=$PWD
    cd ${WorkloadsDir}subs/*$1
    SubsDir=$PWD
    cd ${WorkloadsDir}pubs/*$1
    PubsDir=$PWD
    cd ${OutputDir}*$1
    ResultsDir=$PWD
    cd $CurDir
    for subfile in $(ls $SubsDir); do
        echo -e "\t$subfile"
        OutputSuffix=$1-$subfile-$Suffix
        OutputLog=output-$OutputSuffix
        cmd="$Execname $SubsDir/$subfile $PubsDir/1000.dat >> $ResultsDir/$OutputLog 2>&1"
        rcmd="R -q -e \"x<-read.csv('enclave_CBR_Enclave_Filter/pubs.log',head=F,sep='\t');mean(x[,1]);sd(x[,1])\" >> $ResultsDir/$OutputLog 2>&1"
        echo $cmd
        echo $cmd > $ResultsDir/$OutputLog
        eval $cmd
        echo $rcmd >> $ResultsDir/$OutputLog
        eval $rcmd
        mv $(dirname $Execname)/pubs.log $ResultsDir/Pubs-$OutputSuffix.log
        mv $(dirname $Execname)/subs.log $ResultsDir/Subs-$OutputSuffix.log
    done
}

for jobs in $( ls -d $OutputDir[1-9]*/ | sed 's/\(.*\)\//\1/' ); do
    job=${jobs##*.}
    executewload $job
    echo -e "\n"
done

