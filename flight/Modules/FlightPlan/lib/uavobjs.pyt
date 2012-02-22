import $(IMPORTLIST)
import struct, inspect, datetime, textwrap
objlist = [ $(IMPORTLIST) ]

objid_map = {}
for s in objlist:
    for n,o in inspect.getmembers(s):
	if hasattr(o,'OBJID'):
	    objid_map[ o.OBJID ] = o

class SyncError(AssertionError):
    pass

class ObjidError(KeyError):
    def __init__(self, bad_id):
	self.objID = bad_id

class uavreader:
    header_names = [ 'timestamp', 'datasize', 'sync', 'msgType', 'msgSize', 'objID' ]
    header_types = [ '<I',        '<q',       '<B',   '<B',      '<H',      '<i'    ]
    header_types = map( lambda x: struct.Struct(x), header_types )
    header_zip   = zip(header_names, header_types)
    header_type_map = dict(header_zip)
    sync_code    = 0x3C
    msgType_code = 0x20
    CRC_type     = struct.Struct( '<B' )
    sync_offset  = sum(map(lambda x: x.size, header_types[0:header_names.index('sync')]))
    
    # calculate header size
    _i = 0
    for _n,_t in header_zip:
	_i += _t.size
    header_size = _i

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
	    
    next_chunk_size      = 100
    next_chunk_pack_code = struct.Struct('%s%d%s' %
	(header_type_map['sync'].format[0], next_chunk_size, header_type_map['sync'].format[1]))

    def next_uavobject(self, data, idx):
	# Assumes corrupt object at idx, so begins by skipping a full header
        lasti = idx + self.header_size
        
        # first search for syncs in block of next_chunk_size
        for i in range(lasti, len(data) - 1 - self.next_chunk_size, self.next_chunk_size):
            vs = self.next_chunk_pack_code.unpack_from(data, i)
            lasti = i
            for i2, v in enumerate(vs):
                if v == self.sync_code:
                    return i + i2 - self.sync_offset
                    
        # search for syncs one by one                    
	for i in range(lasti, len(data) - 1):
            v = header_type_map['sync'].unpack_from(data,i)[0]
	    if v == self.sync_code:
		return i - self.sync_offset
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
		i = self.next_uavobject(data,i)
	    except ObjidError as e:
		print '# bad object %d at %d' %(e.objID, i)
		i = self.next_uavobject(data,i)
		
	return ret, i
	
    def dump_uav_list_mat_text(self, list):
	out   = '# Created by OP UAV tools, %s\n' % (datetime.datetime.now().strftime('%a %b %d %H:%M:%S %Y'))
	types = set(map(lambda x: x['obj'].__class__.__name__, list))
	for t in types:
	    fi = filter( lambda x: x['obj'].__class__.__name__ == t, list)
	    demo_obj = fi[0]['obj']
	    data_out = [ ('timestamp', map(lambda x: x['timestamp'], fi)) ]
	    if not demo_obj.isSingleInst:
		data_out.append(('instanceID', map(lambda x: x['obj'].instId, fi)))
	    for n,f in enumerate(demo_obj.fields): # use item 0 as model
		if type(f.value) == type(0) or type(f.value) == type(0.0):
		    data_out.append((f.name, map(lambda x: x['obj'].fields[n].value, fi)))

	    out += textwrap.dedent("""\
		# name: %s
		# type: struct
		# length: %d
		""") % (t, len(data_out))

	    for name,data in data_out:
		strs = map(lambda x: '%g' % x, data)
                out  += textwrap.dedent("""\
		    # name: %s
		    # type: cell
		    # rows: 1
		    # columns: 1
		    # name: <cell-element>
		    # type: matrix
		    # rows: 1
		    # columns: %d
		    """) % (name, len(strs))
		out    += " ".join(strs) + "\n\n"
         
        return out
         
if __name__ == "__main__":
    import sys
    opl_filename = sys.argv[1]
    f            = open(opl_filename)
    data         = f.read()
    r            = uavreader()
    T, idx       = r.unpack(data)
    print r.dump_uav_list_mat_text(T)
