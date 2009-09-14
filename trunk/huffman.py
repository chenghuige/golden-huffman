'''
Create a huffman tree from 
the input is a list like
[('a',3), ('b',2)]
frequnce of 'a' appeard is stored as it's weight
'''
from Queue import PriorityQueue
#if do not use treeWiter so not include pygraphviz than can use py3.0
from treeWriter import TreeWriter
from copy import copy

class NodeBase():
    def __init__(self):
        self.weight = 0
        
    def elem(self):
        return self.weight
    
class Node(NodeBase):
    def __init__(self, weight = 0, left = None, right = None):
        self.weight = weight
        self.left = left
        self.right = right
    
    def __str__(self):
        return str(self.weight)
    
class Leaf(NodeBase):
    def __init__(self, key = '', weight = 0):
        self.key = key
        self.weight = weight
        
    def __str__(self):
        return str(self.key)
    
 
def convert(c):
    '''
    input c = 'a' ord(a) = 97
    bin(97) = '0b1100001'
    return ['0', '1', '1', '0', '0', '0', '0', '1']
    '''    
    l1 = list(bin(ord(c))) #like 0b11101
    l2 = ['0'] * (10 - len(l1))
    l2.extend(l1[2:])
    return l2     
       
class HuffmanTree():
    '''
    base class for HuffmanTreeForCompress and HuffmanTreeForDecompress
    '''
    def __init__(self):     
        self.root = None

class HuffmanTreeForCompress(HuffmanTree):   
    '''
    create a huffman tree for the compressing process
    here self.list like [('a',3),('b',4)...] where 'a' is key, 3 is weight
    or say frequence of 'a' appear in the text
    '''         
    def __init__(self, list):
        HuffmanTree.__init__(self)
        self.list = list #like [('a',3),('b',4)...]
        self.dict = {} #like {'a':[0,1,1,0] , ....}
        
        self.__buildTree()
        self.__genEncode()
        
    def __initPriorityQueue(self, queue):
        '''
        init priority queue let lowest weight at top
        '''
        for key, weight in self.list:
            leaf = Leaf(key, weight)
            queue.put((weight,leaf))
            
    def __buildTree(self):
        '''
        build the huffman tree from the list of weight using prority queue
        greedy alogrithm,choose two least frequence node first
        '''
        length = len(self.list)
        queue = PriorityQueue(length)
        self.__initPriorityQueue(queue)
        #while queue.qsize() > 1:
        # do len(self.list) - 1 times same as while queue.qsize() > 1
        for i in range(length - 1): 
            left = queue.get()[1]
            right = queue.get()[1]
            weight = left.weight + right.weight
            node = Node(weight, left, right)
            queue.put((weight,node))
        self.root = queue.get()[1]
        
    def __genEncode(self):   
        '''
        get huffman encode for each key using depth first travel of tree
        '''  
        def genEncodeHelp(root, encode = []):
            if isinstance(root, Leaf):
                #TODO notice need copy content here,why can't list(encode)?
                self.dict[root.key] = copy(encode)
                #print self.dict[root.key]
                return
            encode.append(0)
            genEncodeHelp(root.left, encode)
            encode[len(encode) - 1] = 1
            genEncodeHelp(root.right, encode)
            encode.pop()
        genEncodeHelp(self.root)
        

class HuffmanTreeForDecompress(HuffmanTree):
    '''
    rebuild of huffman tree for the decompressing process 
    '''    
    def __init__(self, infile):
        HuffmanTree.__init__(self)
        self.__buildTree(infile)
    
    def __buildTree(self, infile):
        def buildTreeHelp(infile):
            first = infile.read(1)
            second = infile.read(1)
            #if not (first == '\xff' and second == '\xfe'):  #is leaf
            if first == '\x00':  #is leaf, not consider unicode now
                return Leaf(second)
            node = Node()
            node.left = buildTreeHelp(infile)
            node.right = buildTreeHelp(infile)
            return node
        infile.read(2)
        self.root = Node()
        self.root.left = buildTreeHelp(infile)
        self.root.right = buildTreeHelp(infile)

class Decompress():
    def __init__(self, infileName, outfileName = ''):
        #TODO better name, expection of opening file
        self.infile = open(infileName, 'rb')
        if outfileName == '':
            outfileName = infileName + '.de'
        self.outfile = open(outfileName, 'wb')
        self.tree = None
        
    def __del__(self):
        self.infile.close()
        self.outfile.close()    
        
    def decompress(self):
        self.__rebuildHuffmanTree()
        self.__decodeFile()        
        
    def __rebuildHuffmanTree(self):
        self.infile.seek(0)
        self.tree = HuffmanTreeForDecompress(self.infile)
        #HuffmanTreeWriter(self.tree).write('tree2.png') #for debug
        
    def __decodeFile(self):
        #right now do not consier speed up using table
        #do not consider the last byte since it's wrong right now
        
        #TODO use a table as 0x00 -> 0000 0000  will speed up?
        self.outfile.seek(0)
        leftBit = ord(self.infile.read(1))
        lastByte = self.infile.read(1)   #it is the last byte if leftBit != 0
        curNode = self.tree.root
        #import gc
        #gc.disable()
        while 1:
            c = self.infile.read(1) #how about Chinese caracter? 2 bytes?
            if c == '':
                break
            li = convert(c) #in c++ you can not return refernce to local in func here ok? yes
            for x in li:
                if x == '0':
                    curNode = curNode.left
                else:
                    curNode = curNode.right
                if isinstance(curNode, Leaf): #the cost of isinstance is higer than lkie root.left == None ?
                    self.outfile.write(curNode.key)   
                    curNode = self.tree.root  
         
        
        #deal with the last bye if leftBit != 0
        #TODO notcice code repeate can we improve?
        if leftBit:
            li = convert(lastByte)
            for x in li:
                if x == '0':
                    curNode = curNode.left
                else:
                    curNode = curNode.right
                if isinstance(curNode, Leaf): #the cost of isinstance is higer than lkie root.left == None ?
                    self.outfile.write(curNode.key)   
                    curNode = self.tree.root 
                    break    #for the last byte if we find one than it's over,the other bits are useless
                
        self.outfile.flush()
        #gc.enable()
                
              
    
class Compress():
    def __init__(self, infileName, outfileName = ''):
        self.infile = open(infileName, 'rb')
        if outfileName == '':
            outfileName = infileName + '.compress'
        self.outfile = open(outfileName, 'wb')
        self.dict = {}
        self.tree = None
     
    def __del__(self):
        self.infile.close()
        self.outfile.close()
        
    def compress(self):
        self.__caculateFrequence()
        self.__createHuffmanTree()
        self.__writeCompressedFile()        
    
    def __caculateFrequence(self):
        '''
        The first time of reading the input file and caculate each
        character frequence store in self.dict
        '''
        self.infile.seek(0)
        while 1:
            c = self.infile.read(1) #how about Chinese caracter? 2 bytes?
            if c == '':
                break
            #print c
            if c in self.dict:
                self.dict[c] += 1
            else:
                self.dict[c] = 0
    
    def __createHuffmanTree(self):
        '''
        Build a huffman tree from self.dict.items()
        '''
        #TODO for py 3.0 need list(self.dict.items()) instead
        self.tree = HuffmanTreeForCompress(list(self.dict.items()))
        #HuffmanTreeWriter(self.tree).write('tree1.png') #for debug
        
    def __writeCompressedFile(self):
        '''
        Create the compressed file 
        First write the huffman tree to the head of outfile
        than translate the input file with encode and write the result to
        outfile
        '''
        self.outfile.seek(0)
        self.__serializeTree()
        self.__encodeFile()
        
    def __serializeTree(self):
        '''
        In order to write the tree like node node leaf node ....
        in pre order sequence to the compressed file head
        here will return the sequence list
        TODO  reuse pre order and using decorator technic!!
        list like [(0,0), (0,0), (1,'c')...], 
        (0,0) the first 0 means internal node
        (1,'c') the first 1 means leaf and 'c' is the key
        '''
        def serializeTreeHelp(root, mfile):
            if isinstance(root, Leaf):
                mfile.write('\x00') #0x0
                mfile.write(root.key)
                return
            mfile.write('\xff') #'\xff' is one character representing 0xff
            mfile.write('\xfe') #0xfe
            serializeTreeHelp(root.left, mfile)
            serializeTreeHelp(root.right, mfile)
        serializeTreeHelp(self.tree.root, self.outfile)
            
    
    def __encodeFile(self):
        '''
        The second time of reading input file
        translate the input file with encode and write the result to outfile 
        TODO can this be improved speed up?
        just write \xff as \b 1111 1111 ? can this be possible so do not need 
        to caculate 255 than translate to \xff and write?
        '''
        self.infile.seek(0)
        #save this pos we will write here later
        pos = self.outfile.tell()
        self.outfile.write(chr(0))  #store left bit
        self.outfile.write(chr(0))  #if left bit !=0 this is the last byte
        num = 0
        i = 0;
        while 1:
            c = self.infile.read(1) #how about Chinese caracter? 2 bytes?
            if c == '':
                break
            li = self.tree.dict[c]
            for x in li:
                num = (num << 1) + x 
                i += 1
                if (i == 8):
                    self.outfile.write(chr(num))
                    num = 0
                    i = 0
        #for all left bit we will fill with 0,and fil finally save left bit
        #like the last is 11 wich has 6 bits left than will store the last
        #byte as 1100,0000
        leftBit = (8 - i)%8  
        if leftBit: 
            for j in range(i, 8):
                num = (num << 1)
                
        #just after the huffman tree sotre how many bits are left for last 
        #byte that is not used and filled with 0
        self.outfile.seek(pos)
        self.outfile.write(chr(leftBit))  #still wrong can't not read well
        self.outfile.write(chr(num))
        self.outfile.flush()  #well need this, why? remember !!!!
        #self.outfile.seek(0,2)   #will not write success without this a bug???
        #print self.outfile.read(1)
        
        
        
 #   def test(self):
 #       for k, v in self.dict.items():
 #           print k
 #           print v
            

class HuffmanTreeWriter(TreeWriter):
    '''
    draw a huffman tree to tree.png or user spcified file
    For huffman debug only
    '''
    def writeHelp(self, root, A):
        p = str(self.num)
        self.num += 1
        
        if isinstance(root, Leaf):
            key = root.key  #TODO '\n' wrong to fix
            #key.replace('\n', '\\n')
            #A.add_node(p, label = str(root.elem()) + r'\n' + key, shape = 'rect')
            A.add_node(p, label = str(root.elem()) + r'\n', shape = 'rect')
            return p
        
        #if not a leaf for huffman tree it must both have left and right child
        A.add_node(p, label = str(root.elem()))

        q = self.writeHelp(root.left, A)
        A.add_node(q, label = str(root.left.elem()))
        A.add_edge(p, q, label = '0')
       
        r = self.writeHelp(root.right, A)
        A.add_node(r, label = str(root.right.elem()))
        A.add_edge(p, r, label = '1')
            
        l = str(self.num2)
        self.num2 -= 1
        A.add_node(l, style = 'invis')
        A.add_edge(p, l, style = 'invis')
        B = A.add_subgraph([q, l, r], rank = 'same')
        B.add_edge(q, l, style = 'invis')
        B.add_edge(l, r, style = 'invis')
        
        return p  #return key root node        




if __name__ == '__main__':
    #d = [chr(ord('a')+i) for i in range(13)]
    #w = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41]
    #list = []
    #for i in range(13):
    #    list.append((d[i], w[i]))
    #print(list)
    #tree = HuffmanTreeForCompress(list)
    #writer = HuffmanTreeWriter(tree)
    #writer.write()
    #tree.test()
    import sys
    if len(sys.argv) == 1:
        inputFileName = 'test.log'
    else:
        inputFileName = sys.argv[1]
    compress = Compress(inputFileName)
    compress.compress()
    
    decompress = Decompress(inputFileName + '.compress')
    decompress.decompress()
        
    #compress.test()
