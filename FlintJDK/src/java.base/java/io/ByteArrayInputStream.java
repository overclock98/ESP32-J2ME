package java.io;

public class ByteArrayInputStream extends InputStream {
    protected byte[] buf;
    protected int pos;
    protected int mark = 0;
    protected int count;
    public ByteArrayInputStream(byte[] buf) { this.buf = buf; this.pos = 0; this.count = buf.length; }
    public ByteArrayInputStream(byte[] buf, int offset, int length) {
        this.buf = buf; this.pos = offset; this.count = Math.min(offset + length, buf.length); this.mark = offset;
    }
    public synchronized int read() { return (pos < count) ? (buf[pos++] & 0xFF) : -1; }
    public synchronized int read(byte[] b, int off, int len) {
        if (b == null) throw new NullPointerException();
        if (pos >= count) return -1;
        int avail = count - pos;
        if (len > avail) len = avail;
        if (len <= 0) return 0;
        System.arraycopy(buf, pos, b, off, len);
        pos += len;
        return len;
    }
    public synchronized long skip(long n) {
        long k = count - pos;
        if (n < k) k = n;
        if (k < 0) k = 0;
        pos += (int) k;
        return k;
    }
    public synchronized int available() { return count - pos; }
    public boolean markSupported() { return true; }
    public synchronized void mark(int readAheadLimit) { mark = pos; }
    public synchronized void reset() { pos = mark; }
    public void close() throws IOException {}
}
