#import sys
#from binaryTree import *
#right pygraphviz does not support py3.0
import pygraphviz as pgv
'''
treeWriter with func wite can write a binary tree to tree.png or user spcified
file
'''
class TreeWriter():
    def __init__(self, tree):
        self.num = 1       #mark each visible node as its key
        self.num2 = -1     #makk each invisible node as its key
        self.tree = tree
        
    def write(self, outfile = 'tree.png'):
        self.A = pgv.AGraph(directed=True,strict=True)
        self.writeHelp(self.tree.root, self.A)
        self.A.graph_attr['epsilon']='0.001'
        print self.A.string() # print dot file to standard output
        self.A.layout('dot') # layout with dot
        self.A.draw(outfile) # write to file     
    
    #well seems should be __writeHelp,but for sub class it could not cover
    #__writeHelp of parent class:( due to __TreeWriter__writeHelp
    #see http://blog.csdn.net/lanphaday/archive/2007/08/09/1734990.aspx
    def writeHelp(self, root, A): 
        p = str(self.num)
        self.num += 1
        A.add_node(p, label = str(root.elem()))
        q = None
        r = None
        
        if root.left:
            q = self.writeHelp(root.left, A)
            A.add_node(q, label = str(root.left.elem()))
            A.add_edge(p,q)
        if root.right:
            r = self.writeHelp(root.right, A)
            A.add_node(r, label = str(root.right.elem()))
            A.add_edge(p,r)
            
        if q or r:
            if not q:
                q = str(self.num2)
                self.num2 -= 1
                A.add_node(q, style = 'invis')
                A.add_edge(p, q, style = 'invis')
            if not r:
                r = str(self.num2)
                self.num2 -= 1
                A.add_node(r, style = 'invis')
                A.add_edge(p, r, style = 'invis')
            l = str(self.num2)
            self.num2 -= 1
            A.add_node(l, style = 'invis')
            A.add_edge(p, l, style = 'invis')
            B = A.add_subgraph([q, l, r], rank = 'same')
            B.add_edge(q, l, style = 'invis')
            B.add_edge(l, r, style = 'invis')
        
        return p  #return key root node    
        
  
#if __name__ == '__main__':
#    tree = BinaryTree()
#    tree.createTree(-1)
    #tree.inorderTravel()
#    writer = TreeWriter(tree)
#    if len(sys.argv) > 1:
#        outfile = sys.argv[1]
#        writer.write(outfile) #write result to outfile
#    else:
#        writer.write() #write result to tree.png
    
