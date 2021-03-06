from ovito.io import import_file
from ovito.data import NearestNeighborFinder

# Load input simulation file.
pipeline = import_file("simulation.dump")
data = pipeline.source
positions = data.particle_properties['Position']

# Initialize neighbor finder object.
# Visit the 12 nearest neighbors of each particle.
N = 12
finder = NearestNeighborFinder(N, data)

# Loop over all input particles:
for index in range(len(positions)):
    print("Nearest neighbors of particle %i:" % index)
    # Iterate over the neighbors of the current particle, starting with the closest:
    for neigh in finder.find(index):
        print(neigh.index, neigh.distance, neigh.delta)

# Find particles closest to some spatial point (x,y,z):
coords = (0, 0, 0)
for neigh in finder.find_at(coords):
    print(neigh.index, neigh.distance, neigh.delta)
