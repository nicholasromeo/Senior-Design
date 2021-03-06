""" 
This module provides functions for interfacing with the ASE (`Atomistic Simulation Environment <https://wiki.fysik.dtu.dk/ase/>`_).
It contains two high-level functions for converting atomistic data back and forth between 
OVITO and ASE:

    * :py:func:`ovito_to_ase`
    * :py:func:`ase_to_ovito`

.. note::

    Using the functions of this module will raise an ``ImportError`` if the ASE package 
    is not installed in the current Python interpreter. Note that the built-in
    Python interpreter of OVITO does *not* contain the ASE package by default.
    It is therefore recommended to build OVITO from source (as explained in the user manual),
    allowing you to import all packages available to the system Python interpreter.

"""

import numpy as np

from ...data import DataCollection, SimulationCell, ParticleProperty, ParticleType

__all__ = ['ovito_to_ase', 'ase_to_ovito']

def ovito_to_ase(data_collection):
    """
    Constructs an `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`_ from the 
    particle data in an existing OVITO :py:class:`~ovito.data.DataCollection`.

    :param: data_collection: The OVITO :py:class:`~ovito.data.DataCollection` to convert.
    :return: An `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`_ containing the
             converted particle data from the source :py:class:`~ovito.data.DataCollection`.

    Usage example:

    .. literalinclude:: ../example_snippets/ovito_to_ase.py
       :lines: 6-
             
    """

    from ase.atoms import Atoms
    from ase.data import chemical_symbols

    # Extract basic data: pbc, cell, positions, particle types
    cell_obj = data_collection.find(SimulationCell)
    pbc = cell_obj.pbc if cell_obj is not None else None
    cell = cell_obj[:, :3].T if cell_obj is not None else None    
    info = {'cell_origin': cell_obj[:, 3] }if cell_obj is not None else None
    positions = np.asarray(data_collection.particle_properties['Position'])
    if 'Particle Type' in data_collection.particle_properties:
        # ASE only accepts chemical symbols as atom type names.
        # If our atom type names are not chemical symbols, pass the numerical atom type to ASE instead.
        type_names = {}
        for t in data_collection.particle_properties['Particle Type'].types:
            if t.name in chemical_symbols:
                type_names[t.id] = t.name
            else:
                type_names[t.id] = t.id
        symbols = [type_names[id] for id in data_collection.particle_properties['Particle Type']]
    else:
        symbols = None
    
    # Construct ase.Atoms object
    atoms = Atoms(symbols,
                  positions=positions,
                  cell=cell,
                  pbc=pbc,
                  info=info)

    # Convert any other particle properties to additional arrays
    for name, prop in data_collection.particle_properties.items():
        if name in ['Position',
                    'Particle Type']:
            continue
        if not isinstance(prop, ParticleProperty):
            continue
        prop_name = prop.name
        i = 1
        while prop_name in atoms.arrays:
            prop_name = '{0}_{1}'.format(prop.name, i)
            i += 1
        atoms.new_array(prop_name, np.asanyarray(prop))

    return atoms

def ase_to_ovito(atoms, data_collection):
    """
    Converts an `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`_ to an OVITO :py:class:`~ovito.data.DataCollection`.

    The second function parameter specifies the destination data collection that will be filled with the converted
    atomistic data from the ASE Atoms object. You can pass any object that implements the :py:class:`~ovito.data.DataCollection` interface, e.g.
    a :py:class:`~ovito.data.PipelineFlowState` or a :py:class:`~ovito.pipeline.StaticSource`. Any existing data in *data_collection* is removed first. 

    :param atoms: The `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`_ to be converted.
    :param data_collection: An object supporting the :py:class:`~ovito.data.DataCollection` interface that receives the output.

    Usage example:

    .. literalinclude:: ../example_snippets/ase_to_ovito.py
       :lines: 6-
    
    """
    assert(isinstance(data_collection, DataCollection))

    # Clear the data collection first before filling it with the converted atoms data.
    del data_collection.objects[:]

    # Set the unit cell and origin (if specified in atoms.info)
    cell = SimulationCell()
    cell.pbc = [bool(p) for p in atoms.get_pbc()]
    with cell.modify() as matrix: 
        matrix[:, :3] = atoms.get_cell().T
        matrix[:, 3]  = atoms.info.get('cell_origin', [0., 0., 0.])
    data_collection.objects.append(cell)

    # Add ParticleProperty from atomic positions
    data_collection.particle_properties.create(ParticleProperty.Type.Position, data=atoms.get_positions())

    # Set particle types from chemical symbols
    types = data_collection.particle_properties.create(ParticleProperty.Type.ParticleType)
    symbols = atoms.get_chemical_symbols()
    type_list = list(set(symbols))
    for i, sym in enumerate(type_list):
        types.type_list.append(ParticleType(id=i+1, name=sym))
    with types.modify() as arr:
        for i,sym in enumerate(symbols):
            arr[i] = type_list.index(sym)+1

    # Check for computed properties - forces, energies, stresses
    calc = atoms.get_calculator()
    if calc is not None:
        for name, ptype in [('forces', ParticleProperty.Type.Force),
                            ('energies', ParticleProperty.Type.PotentialEnergy),
                            ('stresses', ParticleProperty.Type.StressTensor),
                            ('charges', ParticleProperty.Type.Charge)]:
            try:
                array = calc.get_property(name,
                                          atoms,
                                          allow_calculation=False)
                if array is None:
                    continue
            except NotImplementedError:
                continue

            # Create a corresponding OVITO standard property.
            data_collection.particle_properties.create(ptype, data=array)

    # Create extra properties in DataCollection
    for name, array in atoms.arrays.items():
        if name in ['positions', 'numbers']:
            continue
        data_collection.particle_properties.create(name, data=array)
