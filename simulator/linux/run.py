#!/usr/bin/env python3

import os
import sys
import argparse

model_bin = "model.out";
compile_file = "compile.sh";
recompile = False;

# validation functoin for the argparser
def file_exists(filepath):
    return os.path.isfile(filepath)

# validation functoin for the argparser
def is_dir(dirpath):
    if os.path.isdir(dirpath):
        return dirpath
    else:
        raise argparse.ArgumentTypeError(f"readable_dir:{path} is not a valid path")

# validation functoin for the argparser
def restricted_float(fx):
    try:
        fx = float(fx)
    except ValueError:
        raise argparse.ArgumentTypeError("%r not a floating-point literal" % (fx,))

    if fx < 0.0:
        raise argparse.ArgumentTypeError("%r is not positive"%(fx,))
    return fx


def main(arguments):

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('input_dir',         help="Input directory where the files are stored",                         type=is_dir)
    parser.add_argument('output_dir',        help="Output directory where the files are stored",                        type=is_dir)
    parser.add_argument('per_rate_file',     help="name of the packet error rate (PER) file",                           type=argparse.FileType('r'))
    parser.add_argument('error_rate_file',   help="name of the error rate file",                                        type=argparse.FileType('r'))
    parser.add_argument('mapping_file',      help="name of the file that has station mapping",                          type=argparse.FileType('r'))
    parser.add_argument('distance_map',      help="name of the file that has station distance between each other \
                                            in the order defined in the mapping ",                                      type=argparse.FileType('r'))
    parser.add_argument('pathloss_map',      help="name of the file that has station pathloss between each other \
                                            in the order defined in the mapping ",                                      type=argparse.FileType('r'))
    parser.add_argument('simulation_params', help="input file with parameters to the simulator",                        type=argparse.FileType('r'))
    parser.add_argument('deterministic',     help="use the same set of seeds for the random number of generator  \
                                            (RNG) between tests")
    parser.add_argument('showgui',           help="show the gui of the simulation")
    parser.add_argument('guistart',          help="start the gui render after this time in milliseconds format: x.xxx", type=restricted_float)
    parser.add_argument('guiend',            help="end the gui render after this time in milliseconds format: x.xxx",   type=restricted_float)

    args = parser.parse_args(arguments)

    # build the parameter string and pass to the program
    args_string = "";
    for arg in vars(args):
        args_string += " " + arg + ":" + getattr(args, arg)

    exec_program = "./" + model_bin + args_string;
    exec(exec_program);



if __name__ == '__main__':
  prog_desc = "This is a simulation model of a wireless network over long"\
      "distances. This program uses the IEEE802.11 standard as a template"\
      " and make certain adjustments to the PHY and MAC protocol stack to"\
      "greatly improve the latency and reliability of packet tranmission"

  parser = argparse.ArgumentParser(description = prog_desc, add_help=False)
  parser.add_argument_group('required arguments')

  # Add a required argument
  parser.add_argument('tfile', help='temp_file contains fake data.',
    metavar=('<temp_file>'), type=str)

  # Add a required argument with limited options
  temp_types = ["hot", "cold", "warm"]
  parser.add_argument('ttemp', help='type of temperatures in the temp_file.',
    choices=temp_types, type=str)

  # Help argument
  parser.add_argument('-h','--help',action='help',default=argparse.SUPPRESS,
    help=argparse._('show this help message and exit.'))

  # Rename the arguements' title. The default (i.e. positional arguments)
  # is a bit confusing to users.
  parser._positionals.title = "required arguments"

  args = parser.parse_args()
  main(args)