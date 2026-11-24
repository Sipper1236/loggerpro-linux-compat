import socket, struct

def exact(c, n):
    data = bytearray()
    while len(data) < n:
        part = c.recv(n - len(data))
        if not part: raise EOFError
        data.extend(part)
    return bytes(data)

with socket.socket() as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('127.0.0.1', 47821)); s.listen(8)
    print('mock listening', flush=True)
    while True:
        c, _ = s.accept()
        with c:
            op = exact(c, 1)
            if op == b'O':
                opening = exact(c, 5)
                print('open', opening.hex(), flush=True)
                c.sendall(b'\0' + struct.pack('<I', 77))
            elif op in (b'W', b'R'):
                session, pipe, size = struct.unpack('<IBI', exact(c, 9))
                if op == b'W':
                    payload = exact(c, size)
                    print('write', session, hex(pipe), payload.hex(), flush=True)
                    c.sendall(b'\0' + struct.pack('<I', size))
                else:
                    payload = bytes.fromhex('deadbeef')[:size]
                    print('read', session, hex(pipe), size, flush=True)
                    c.sendall(b'\0' + struct.pack('<I', len(payload)) + payload)
            elif op == b'C':
                session = struct.unpack('<I', exact(c, 4))[0]
                print('close', session, flush=True); c.sendall(b'\0')
            elif op == b'A':
                session, pipe = struct.unpack('<IB', exact(c, 5))
                print('abort', session, hex(pipe), flush=True); c.sendall(b'\0')
            else:
                raise ValueError(op)
