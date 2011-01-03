/* ------- file: -------------------------- rhf1d.c -----------------

       Version:       rh2.0, 1-D plane-parallel
       Author:        Han Uitenbroek (huitenbroek@nso.edu)
       Last modified: Wed Apr 22 09:45:58 2009 --

       --------------------------                      ----------RH-- */

/* --- Main routine of 1D plane-parallel radiative transfer program.
       MALI scheme formulated according to Rybicki & Hummer

  See: G. B. Rybicki and D. G. Hummer 1991, A&A 245, p. 171-181
       G. B. Rybicki and D. G. Hummer 1992, A&A 263, p. 209-215

       Formal solution is performed with Feautrier difference scheme
       in static atmospheres, and with piecewise quadratic integration
       in moving atmospheres.

       --                                              -------------- */

#include <string.h>

#include "rh.h"
#include "atom.h"
#include "atmos.h"
#include "geometry.h"
#include "spectrum.h"
#include "background.h"
#include "statistics.h"
#include "error.h"
#include "inputs.h"
#include "xdr.h"


/* --- Function prototypes --                          -------------- */


/* --- Global variables --                             -------------- */

enum Topology topology = ONE_D_PLANE;

Atmosphere atmos;
Geometry geometry;
Spectrum spectrum;
ProgramStats stats;
InputData input;
CommandLine commandline;
char messageStr[MAX_MESSAGE_LENGTH];


/* ------- begin -------------------------- rhf1d.c ----------------- */

int main(int argc, char *argv[])
{
  bool_t analyze_output, equilibria_only;
  int    niter, nact,i;

  Atom *atom;
  Molecule *molecule;

  /* --- Read input data and initialize --             -------------- */

  setOptions(argc, argv);
  getCPU(0, TIME_START, NULL);
  SetFPEtraps();

  readInput();
  spectrum.updateJ = TRUE;

  getCPU(1, TIME_START, NULL);
  MULTIatmos(&atmos, &geometry);
  if (atmos.Stokes) Bproject();

  // TIAGO temp
  /*
  printf("    height      temp        ne           vz         vturb\n");
  //printf("    nh(0)       nh(1)       nh(2)        nh(3)      nh(4)        nh(5)       nhtot\n");
  for(i=0; i<175; i++){
    //printf(" %10.4e  %10.4e  %10.4e  %10.4e %10.4e  %10.4e  %10.4e\n",
    //   atmos.nH[0][i],atmos.nH[1][i],atmos.nH[2][i],atmos.nH[3][i],atmos.nH[4][i],
    //   atmos.nH[5][i],atmos.nHtot[i]);

    printf(" %10.4e  %10.4e  %10.4e  %10.4e %10.4e\n",
    	   geometry.height[i],atmos.T[i],atmos.ne[i],geometry.vel[i],atmos.vturb[i]);
  }
  exit(0);
  */
  // TIAGO
  

  readAtomicModels();
  readMolecularModels();
  SortLambda();
  
  getBoundary(&geometry);
  
  Background(analyze_output=TRUE, equilibria_only=FALSE);
  convertScales(&atmos, &geometry);

  getProfiles();
  initSolution();
  initScatter();

  getCPU(1, TIME_POLL, "Total Initialize");

  /* --- Solve radiative transfer for active ingredients -- --------- */

  Iterate(input.NmaxIter, input.iterLimit);

  adjustStokesMode();
  niter = 0;
  while (niter < input.NmaxScatter) {
    if (solveSpectrum(FALSE, FALSE) <= input.iterLimit) break;
    niter++;
  }
  /* --- Write output files --                         -------------- */

  getCPU(1, TIME_START, NULL);

  writeInput();
  writeAtmos(&atmos);
  writeGeometry(&geometry);
  writeSpectrum(&spectrum);
  writeFlux(FLUX_DOT_OUT);

  for (nact = 0;  nact < atmos.Nactiveatom;  nact++) {
    atom = atmos.activeatoms[nact];

    writeAtom(atom);
    writePopulations(atom);
    writeRadRate(atom);
    writeCollisionRate(atom);
    writeDamping(atom);
  } 
  for (nact = 0;  nact < atmos.Nactivemol;  nact++) {
    molecule = atmos.activemols[nact];
    writeMolPops(molecule);
  }

  writeOpacity();

  getCPU(1, TIME_POLL, "Write output");
  printTotalCPU();
}
/* ------- end ---------------------------- rhf1d.c ----------------- */