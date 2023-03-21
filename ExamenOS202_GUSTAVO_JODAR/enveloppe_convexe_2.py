import sys
import numpy as np
import math
import time
import matplotlib.pyplot as plt
from numpy.random import MT19937
from numpy.random import RandomState, SeedSequence

from mpi4py import MPI

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()


class droite:
    def __init__( self, p1, p2):
        self.a =  p2[1] - p1[1]
        self.b = -p2[0] + p1[0]
        self.c =  p1[1] * p2[0] - p1[0] * p2[1]

    def meme_cote(self, q1, q2) -> bool:
        return ( self.a * q1[0] + self.b * q1[1] + self.c ) * ( self.a * q2[0] + self.b * q2[1] + self.c )  > 0

def calcul_enveloppe( nuage_de_points : np.ndarray ) -> np.ndarray :
    enveloppe = []
    lst_nuage = list(nuage_de_points[:])
    # Recherche du point appartenant au nuage ayant l'ordonnée la plus basse.
    lst_nuage.sort(key=lambda coord : coord[1])
    bas = lst_nuage.pop(0)
    # Ce point appartient forcément à l'enveloppe convexe !
    enveloppe.append(bas)

    # On trie le reste du nuage en fonction des angles formés par la droite parallèle à l'abscisse et passant par bas avec
    # la droite reliant bas avec le point considéré
    lst_nuage.sort(key=lambda coord : math.atan2(coord[1]-bas[1], coord[0]-bas[0]))

    # On replace le point le plus bas à la fin de la liste des points du nuage
    lst_nuage.append(bas)

    # Tant qu'il y a des points dans le nuage...
    while len(lst_nuage) > 0:
        # Puisque le premier point a l'angle minimal, il appartient à l'enveloppe :
        enveloppe.append(lst_nuage.pop(0))

        # Tant qu'il y a au moins quatre points dans l'enveloppe...
        while len(enveloppe)>=4:
            if not droite( enveloppe[-3], enveloppe[-2] ).meme_cote( enveloppe[-4], enveloppe[-1] ):
                enveloppe.pop(-2)
            else:
                break
    return np.array(enveloppe)

taille_nuage : int = 55440
nbre_repet   : int =     3
resolution_x : int = 1_000
resolution_y : int = 1_000

# Génération d'un nuage de points

elapsed_generation : float = 0.
elapsed_convexhull : float = 0.

if len(sys.argv) > 1:
    taille_nuage = int(sys.argv[1])
if len(sys.argv) > 2:
    nbre_repet   = int(sys.argv[2])

enveloppe = None
nuage     = None

for r in range(nbre_repet):
    t1 = time.time()
    nuage = np.array(np.array([[resolution_x * i * math.cos(48371.*i)/taille_nuage for i in range(taille_nuage)], [resolution_y * math.sin(50033./(i+1.)) for i in range(taille_nuage)]], dtype=np.float64).T)
    t2 = time.time()
    elapsed_generation += t2 - t1

#chaque un calcule sa nuage des points
local_nuage_size = int(len(nuage)/size)
nuage_local = nuage[rank*(local_nuage_size):(rank+1)*local_nuage_size]

# Calcul de l'enveloppe convexe :
t1 = time.time()
enveloppe = calcul_enveloppe(nuage_local)
t2 = time.time()
elapsed_convexhull += t2 - t1

print(f"Im {rank} -> {nuage_local.shape}")
print(f"Im {rank} -> {enveloppe.shape}")

#getting everyone's enveloppe
les_enveloppes = [None]*size
les_enveloppes[rank] = enveloppe
for r in range(size):
    comm.Bcast(les_enveloppes[r], root=r)

for i in range(size):
    print(len(les_enveloppes[i]))


print(f"Temps pris pour la generation d'un nuage de points : {elapsed_generation/nbre_repet}")
print(f"Temps pris pour le calcul de l'enveloppe convexe : {elapsed_convexhull/nbre_repet}")
print(f"Temps total : {sum((elapsed_generation, elapsed_convexhull))/nbre_repet}")

#concatener et faire le calcul
enveloppe = np.array(enveloppe.tolist() + other_envelop.tolist())
print(f"Before calcul -> Im {rank} -> {enveloppe.shape}")
enveloppe = calcul_enveloppe(enveloppe)
print(f"After cAlcul -> Im {rank} -> {enveloppe.shape}")


if(rank == 0):
    # affichage du nuage :
    plt.scatter(nuage[:,0], nuage[:,1])
    for i in range(len(enveloppe[:])-1):
        plt.plot([enveloppe[i,0],enveloppe[i+1,0]], [enveloppe[i,1], enveloppe[i+1,1]], 'bo', linestyle="-")
    plt.show()

    # validation de non-regression :
    if (taille_nuage == 55440):
        ref = np.loadtxt("enveloppe_convexe_55440.ref")
        try:
            np.testing.assert_allclose(ref, enveloppe)
            print("Verification pour 55440 points: OK")
        except AssertionError as e:
            print(e)
            print("Verification pour 55440 points: FAILED")
