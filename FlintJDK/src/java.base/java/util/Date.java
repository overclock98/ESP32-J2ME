package java.util;

public class Date {
    private long time;
    public Date() { this.time = System.currentTimeMillis(); }
    public Date(long time) { this.time = time; }
    public long getTime() { return time; }
    public void setTime(long time) { this.time = time; }
    public boolean before(Date when) { return time < when.time; }
    public boolean after(Date when) { return time > when.time; }
    public boolean equals(Object obj) { return (obj instanceof Date) && time == ((Date) obj).time; }
    public int hashCode() { return (int) (time ^ (time >>> 32)); }
    public String toString() { return "Date(" + time + ")"; }
}
