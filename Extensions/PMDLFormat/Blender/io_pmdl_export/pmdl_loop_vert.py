'''
This simple class composes a mesh reference and loop index
into an iterable object structure; ideal for constructing
a vertex index
'''

class loop_vert:

    def __init__(self, mesh, loop):
        self.mesh = mesh
        self.loop = loop

    def __hash__(self):
        return (self.mesh, self.loop.index).__hash__()
    
    def __eq__(self, other):
        return self.mesh == other.mesh and self.loop.index == other.loop.index
    
    def __ne__(self, other):
        return self.mesh != other.mesh or self.loop.index != other.loop.index

    def __str__(self):
        return (self.mesh, self.loop.index).__str__()
