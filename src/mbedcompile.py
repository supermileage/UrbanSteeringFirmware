#!/usr/bin/env python3

import subprocess
import os
import argparse

def build_cmd(image, srcs=[]):
    cmd = "docker run --rm -t"
    def add_src(src):
        nonlocal cmd
        abspath = os.path.abspath(srcs)
        cmd += " -v %s:/code/MbedFirmware/%s" % (abspath, os.path.basename(abspath))

    if type(srcs) is dict:
        for src in srcs:
            add_src(src)
    elif type(srcs) is str:
        add_src(srcs)

    cmd += " %s" % image
    return cmd

class Cmd:
    def __init__(self, image='ubcsupermileage/mbed-compiler:latest', name='nucleo', outputDir='.', cow='none', fig=False):
        self.cmd = "docker run --rm -it"
        self.sources = []
        self.image = image
        self.vars = "mbed-os"
        self.name = name
        self.outputDir = os.path.abspath(outputDir)
        self.cow = cow
        self.fig = 'TRUE' if fig else 'FALSE'
    
    def add_src(self, src):
        abspath = os.path.abspath(src)
        self.cmd += " -v \"%s:/code/MbedFirmware/%s\"" % (abspath, os.path.basename(abspath))
        self.vars += ":%s" % os.path.basename(abspath)
    
    def add_srcs(self, srcs):
        for src in srcs:
            self.add_src(src)

    def envs(self):
        return self.vars

    def set_output_name(self, name):
        self.name = name

    def set_output_dir(self, outputDir):
        self.outputDir = os.path.abspath(outputDir)
    
    def __str__(self):
        return "%s -e \"SRCS=%s\" -v \"%s:/output\" -e \"COMPILENAME=%s\" -e \"COW=%s\" -e \"FIG=%s\" \"%s\"" % (self.cmd, self.vars, self.outputDir, self.name, self.cow, self.fig, self.image)


def main():
    # parse arguments
    parser = argparse.ArgumentParser(description="Compile mbed firmware")
    parser.add_argument('-n', '--name', help="name of output binary", default="nucleo")
    parser.add_argument('-s', '--source', help="directories containing source files",action='append', nargs=1)
    parser.add_argument('-i', '--image', help="name of docker image to run", default='ubcsupermileage/mbed-compiler:latest')
    parser.add_argument('-o', '--output', help="directory to place binary after compilation", default=".")

    parser.add_argument('-c', '--cow', help=argparse.SUPPRESS, default='none')
    parser.add_argument('-d', '--dragon', help=argparse.SUPPRESS, action='store_true')
    parser.add_argument('-f', '--fig', help=argparse.SUPPRESS, action='store_true')

    args = parser.parse_args()
    print(args)
    name = args.name
    image = args.image
    sources = []
    if args.source is None:
        sources = ['.']
    else:
        for arg in args.source:
            sources.append(arg[0])
    outputDir = args.output
    
    cow = 'dragon' if args.dragon else args.cow
    fig = args.fig

    cmd = Cmd(image, name, outputDir, cow, fig)
    #cmd.add_srcs(['Examples'])
    cmd.add_srcs(sources)
    print(cmd)

    docker = subprocess.Popen(str(cmd), shell=True)
    docker.wait()

if __name__ == "__main__":
    main()