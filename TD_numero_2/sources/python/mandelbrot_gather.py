# Calcul de l'ensemble de Mandelbrot en python
import numpy as np
from dataclasses import dataclass
from PIL import Image
from math import log
from time import time
import matplotlib.cm
from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank() #process rank
size = comm.Get_size() # n process

@dataclass
class MandelbrotSet:
    max_iterations: int
    escape_radius : float = 2.0

    def __contains__(self, c: complex) -> bool:
        return self.stability(c) == 1

    def convergence(self, c:complex, smooth=False, clamp=True) -> float:
        value = self.count_iterations(c,smooth)/self.max_iterations
        return max(0.0, min(value, 1.0)) if clamp else value

    def count_iterations(self, c: complex,  smooth=False) -> int | float :
        z    : complex
        iter : int

        # On vérifie dans un premier temps si le complexe
        # n'appartient pas à une zone de convergence connue :
        #   1. Appartenance aux disques  C0{(0,0),1/4} et C1{(-1,0),1/4}
        if c.real*c.real+c.imag*c.imag < 0.0625 :
            return self.max_iterations
        if (c.real+1)*(c.real+1)+c.imag*c.imag < 0.0625:
            return self.max_iterations
        #  2.  Appartenance à la cardioïde {(1/4,0),1/2(1-cos(theta))}    
        if (c.real > -0.75) and (c.real < 0.5) :
            ct = c.real-0.25 + 1.j * c.imag
            ctnrm2 = abs(ct)
            if ctnrm2 < 0.5*(1-ct.real/max(ctnrm2,1.E-14)):
                return self.max_iterations
        # Sinon on itère 
        z=0
        for iter in range(self.max_iterations):
            z = z*z + c
            if abs(z) > self.escape_radius:
                if smooth:
                    return iter + 1 - log(log(abs(z)))/log(2)
                return iter
        return self.max_iterations


# On peut changer les paramètres des deux prochaines lignes
mandelbrot_set = MandelbrotSet(max_iterations=50,escape_radius=10)
width, height = 1024, 1024

scaleX = 3./width
scaleY = 2.25/height
convergence = np.empty((width,height),dtype=np.double)

calculer = []
for tests in range(10):
    convergence = np.empty((width,height),dtype=np.double)
    data = []

    deb = time()

    if(rank == 0):
        for i in range(0, size):
            data.append(convergence[:, i*int(height/size):(i+1)*int(height/size)])

    data = comm.scatter(data, root=0)

    # Calcul de l'ensemble de mandelbrot :
    offset = int(height/size)*rank
    
    for y in range(offset, int(height/size)*(rank+1)):
        for x in range(width):
            c = complex(-2. + scaleX*x, -1.125 + scaleY * y)
            data[x,y - offset] = mandelbrot_set.convergence(c,smooth=True) #mudar onde ele recebe [x,y]

    #print(f"Temps du calcul de l'ensemble de Mandelbrot : {fin-deb}")
    
    newData = comm.gather(data,root=0)

    if(rank == 0):
        img = []
        for i in range(0, size):
            img = img + newData[i].T.tolist()
        
        img = np.array(img)    
        image = Image.fromarray(np.uint8(matplotlib.cm.plasma(img)*255))

        fin = time()
        calculer.append(fin-deb)
        
        print(f"Temps de constitution de l'image : {fin-deb}")
        #image.show()

        print(np.array(calculer).mean())