package java.io;

public class DataInputStream extends InputStream implements DataInput {
    protected InputStream in;
    public DataInputStream(InputStream in) { this.in = in; }
    public int read() throws IOException { return in.read(); }
    public int read(byte[] b) throws IOException { return in.read(b, 0, b.length); }
    public int read(byte[] b, int off, int len) throws IOException { return in.read(b, off, len); }
    public int available() throws IOException { return in.available(); }
    public void close() throws IOException { in.close(); }
    public void mark(int readlimit) { in.mark(readlimit); }
    public void reset() throws IOException { in.reset(); }
    public boolean markSupported() { return in.markSupported(); }
    public long skip(long n) throws IOException { return in.skip(n); }
    public final void readFully(byte[] b) throws IOException { readFully(b, 0, b.length); }
    public final void readFully(byte[] b, int off, int len) throws IOException {
        int n = 0;
        while (n < len) { int c = in.read(b, off + n, len - n); if (c < 0) throw new EOFException(); n += c; }
    }
    public final int skipBytes(int n) throws IOException {
        int total = 0, cur;
        while (total < n && (cur = (int) in.skip(n - total)) > 0) total += cur;
        return total;
    }
    public final boolean readBoolean() throws IOException { return readUByte() != 0; }
    public final byte readByte() throws IOException { return (byte) readUByte(); }
    public final int readUnsignedByte() throws IOException { return readUByte(); }
    public final short readShort() throws IOException { return (short) ((readUByte() << 8) | readUByte()); }
    public final int readUnsignedShort() throws IOException { return (readUByte() << 8) | readUByte(); }
    public final char readChar() throws IOException { return (char) ((readUByte() << 8) | readUByte()); }
    public final int readInt() throws IOException { return (readUByte() << 24) | (readUByte() << 16) | (readUByte() << 8) | readUByte(); }
    public final long readLong() throws IOException { return ((long) readInt() << 32) | (readInt() & 0xFFFFFFFFL); }
    public final float readFloat() throws IOException { return Float.intBitsToFloat(readInt()); }
    public final double readDouble() throws IOException { return Double.longBitsToDouble(readLong()); }
    public final String readLine() throws IOException {
        StringBuilder sb = new StringBuilder();
        int c; boolean any = false;
        while ((c = in.read()) >= 0) { any = true; if (c == '\n' || c == '\r') break; sb.append((char) c); }
        return any ? sb.toString() : null;
    }
    public final String readUTF() throws IOException { return readUTF(this); }
    public static String readUTF(DataInput in) throws IOException {
        int utflen = in.readUnsignedShort();
        byte[] bytearr = new byte[utflen];
        char[] chararr = new char[utflen];
        in.readFully(bytearr, 0, utflen);
        int count = 0, cc = 0;
        while (count < utflen) { int c = bytearr[count] & 0xFF; if (c > 127) break; count++; chararr[cc++] = (char) c; }
        while (count < utflen) {
            int c = bytearr[count] & 0xFF; int x = c >> 4;
            if (x <= 7) { count++; chararr[cc++] = (char) c; }
            else if (x == 12 || x == 13) { count += 2; if (count > utflen) throw new IOException(); int c2 = bytearr[count - 1]; chararr[cc++] = (char) (((c & 0x1F) << 6) | (c2 & 0x3F)); }
            else if (x == 14) { count += 3; if (count > utflen) throw new IOException(); int c2 = bytearr[count - 2]; int c3 = bytearr[count - 1]; chararr[cc++] = (char) (((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F)); }
            else throw new IOException();
        }
        return new String(chararr, 0, cc);
    }
    private int readUByte() throws IOException { int ch = in.read(); if (ch < 0) throw new EOFException(); return ch; }
}
