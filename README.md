##Assasim

AcceSSible Agent-based SIMulator
http://assasim.org/


###Description

Assasim is a project whose goal is to build an open source, easy to use and distributed agent-based modeling software. This project began as a collaborative research project from 14 students of the École Normale Supérieure de Lyon. If you are interested by some technical aspects, please have a look at the technical overview (on our website) or at the sources.


###Platforms support

- Linux: Tested (Ubuntu, ArchLinux)
- MaxOSX: Should work
- Windows: Not yet (almost everything is cross-platform, but some work is needed to compile and to adapt convenience scripts)



###Dependances

- Any MPI 3.0 complient MPI implementation (for example OpenMPI 1.10+, MPICH 3.0.0+ or Intel MPI 5.0+)
- CMake 2.6+
- Boost libraries (1.58+, may work with previous versions)
- LibClang >= 3.6
- Readline for the cli



###How to use

The main respository includes 2 folders: precompilation and cli.

####Configuration (first time only)

You need to configure the sources and the scripts for your computer.

1. To begin with, run the installation script (<code>bash install.sh</code>) which configure the Jeayeson library and create the precompilation executable into directory <code>precompilation/bin/</code>
2. Go into the parent folder (<code>cd ..</code>) and open the <code>simulation-dev.sh</code> file in your favorite text editor; set the content of the <code>EDITOR</code> variable to be the command to execute the text editor that you want to use to input C++ code; finally set the <code>MODE</code> variable depending on your preferences (see the comments in the file for more details, nevertheless we recommend the sync mode (0) for the command line editors and the async mode (1) for graphical editors)
3. Open the <code>precompilation.sh</code> file in your favorite text editor and set the <code>STANDARD_DIR</code> variable to the path to the standard C++ library headers on your computer (it should look like <code>/usr/include/c++/x.x.x/</code>)
4. Add the rights for <code>precompilation.sh"</code> and <code>simulation-dev.sh</code> to execute (<code>chmod +x precompilation.sh simulation-dev.sh</code>)


####Precompilation

The precompilation folder contains everything that is required to build the simulation for your model.

1. Run <code>./simulation-dev.sh < yourhppfile ></code> where <code>< yourhppfile ></code> is a header C++ file; your text editor will open it with a minimal code
2. See tutorial on http://www.assasim.org/Tutorial for defining your model

####Command-line interface

The command-line interface (CLI) allows to take control of the compiled simulation.

<code>./assim-cli [< simulation_executable > < number_of_masters >]</code>

If the two parameters are given, the simulation is automatically spawned and the command-line is already functional.
Otherwise, an ID is given that must be used to manually spawn the simulation executable with this ID as parameter.

To get the list of the commands you can use, start the CLI and type the command <code>help</code>.


####Input and output files format

See the examples in the folder <code>examples</code>.


###Credits

Grégoire Beaudoire
Titouan Carette
Maverick Chardet
Maxime Faron
Jean-Yves Franceschi
Rémy Grünblatt
Antonin Lambilliotte
Fabrice Lebeau
Christophe Lucas
Vincent Michielini
Victor Mollimard
Harold Ndangang Yampa
Clément Sartori
Nicolas Vidal
