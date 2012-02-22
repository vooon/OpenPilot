import $(IMPORTLIST)
import struct, inspect
objlist = [ $(IMPORTLIST) ]

objid_map = {}
for s in objlist:
    for n,o in inspect.getmembers(s):
	if hasattr(o,'OBJID'):
	    exec( 'from %s import %s' % (s.__name__, n))
	    objid_map[ o.OBJID ] = o

class SyncError(AssertionError):
    pass

class ObjidError(KeyError):
    def __init__(self, bad_id):
	self.objID = bad_id

class uavreader:
    header_names = [ 'timestamp', 'datasize', 'sync', 'msgType', 'msgSize', 'objID' ]
    header_types = [ '=I',        '=q',       '=B',   '=B',      '=H',      '=i'    ]
    header_types = map( lambda x: struct.Struct(x), header_types )
    header_zip   = zip(header_names, header_types)
    sync_code    = 0x3C
    msgType_code = 0x20
    CRC_type     = struct.Struct( '=B' )

    def unpack_one(self, data, idx):
	    S      = {}
	    i      = idx
	    for n,t in self.header_zip:
                j  = i + t.size
                S[n] = t.unpack_from(data, i)[0]
		i  = j
		
	    if S['sync'] != self.sync_code or S['msgType'] != self.msgType_code:
		raise SyncError()
	    if S['objID'] not in objid_map:
		raise ObjidError(S['objID'])
	    
	    c        = objid_map[S['objID']]
	    o        = c()
	    i        = o.unpack(data, i)
	    S['obj'] = o
            S['CRC'] = self.CRC_type.unpack_from(data, i)[0]
	    
	    return S, i + self.CRC_type.size
	    
    next_chunk_size = 100
    next_chunk_pack_code = struct.Struct('=100B')  # should really generate from size and header_types

    def next_uavojbect(self, data, idx):
	# Assumes corrupt object at idx, so begins by skipping a full header
	i = 0
	for n,t in self.header_zip:
	    if n == 'sync': 
	        sync_offset = i
	        sync_pc     = t
            i += t.size
	header_size = i
        lasti = idx + header_size
        for i in range(lasti, len(data) - 1 - self.next_chunk_size, self.next_chunk_size):
            vs = self.next_chunk_pack_code.unpack_from(data, i)
            lasti = i
            for v in vs:
                if v == self.sync_code:
                    return i - sync_offset
	for i in range(lasti, len(data) - 1):
            v = sync_pc.unpack_from(data,i)[0]
	    if v == self.sync_code:
		return i - sync_offset
	return len(data)      
	    
    def unpack(self, data, idx = 0):
	ret    = []
	i      = idx
	while i < len(data):
	    try:
	        S,i  = self.unpack_one(data,i)
	        ret.append(S)
	    except SyncError:
		print '# bad sync at %d' % i
		i = self.next_uavojbect(data,i)
	    except ObjidError as e:
		print '# bad object %d at %d' %(e.objID, i)
		i = self.next_uavojbect(data,i)
            if len(ret) % 100 == 0: print len(ret), i
		
	return ret, i
	
    def dump_uav_list_mat_text(self, list):
	out   = "# Created by OP UAV tools\n"
	"""	
	# name: AccessoryDesired
	# type: struct
	# length: 3
	# name: timestamp
	# type: cell
	# rows: 1
	# columns: 1
	# name: <cell-element>
	# type: matrix
	# rows: 1
	# columns: 2381
	"""

	types = set(map(lambda x: x['obj'].__class__.__name__, list))
	for t in types:
	    fi = filter( lambda x: x['obj'].__class__.__name__ == t, list)
	    demo_obj = fi[0]['obj']
	    out = out + """# name: %s
# type: struct
# length: %d
""" % (t, len(demo_obj.fields) + 1)

            out = out + """# name: %s
# type: cell
# rows: 1
# columns: 1
# name: <cell-element>
# type: matrix
# rows: 1
# columns: %d
""" % ('timestamp', len(fi))
            out         = out + " ".join(map( lambda x: '%g' % (x['timestamp']), fi)) + "\n\n"
          
            for n,f in enumerate(demo_obj.fields): # use item 0 as model
                if type(f.value) == type(0) or type(f.value) == type(0.0):
		    values = map(lambda x: x['obj'].fields[n].value, fi)
		    strs   = map(lambda x: '%g' % x, values)
		else:
		    strs   = []
                out = out + """# name: %s
# type: cell
# rows: 1
# columns: 1
# name: <cell-element>
# type: matrix
# rows: 1
# columns: %d
""" % (f.__class__.__name__, len(strs))
		out    = out + " ".join(strs) + "\n\n"
         
        return out
         
if __name__ == "__main__":
    import sys
    opl_filename = sys.argv[1]
    f            = open(opl_filename)
    data         = f.read()
    r            = uavreader()
    T, idx       = r.unpack(data)
    print r.dump_uav_list_mat_text(T)
