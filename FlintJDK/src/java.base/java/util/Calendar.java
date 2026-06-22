package java.util;

public class Calendar {
    public static final int ERA = 0, YEAR = 1, MONTH = 2, WEEK_OF_YEAR = 3, WEEK_OF_MONTH = 4,
        DATE = 5, DAY_OF_MONTH = 5, DAY_OF_YEAR = 6, DAY_OF_WEEK = 7, DAY_OF_WEEK_IN_MONTH = 8,
        AM_PM = 9, HOUR = 10, HOUR_OF_DAY = 11, MINUTE = 12, SECOND = 13, MILLISECOND = 14,
        ZONE_OFFSET = 15, DST_OFFSET = 16, FIELD_COUNT = 17;
    public static final int JANUARY = 0, FEBRUARY = 1, MARCH = 2, APRIL = 3, MAY = 4, JUNE = 5,
        JULY = 6, AUGUST = 7, SEPTEMBER = 8, OCTOBER = 9, NOVEMBER = 10, DECEMBER = 11;
    public static final int SUNDAY = 1, MONDAY = 2, TUESDAY = 3, WEDNESDAY = 4, THURSDAY = 5,
        FRIDAY = 6, SATURDAY = 7;
    public static final int AM = 0, PM = 1;

    protected long time;
    protected Calendar() { this.time = System.currentTimeMillis(); }
    public static Calendar getInstance() { return new Calendar(); }
    public final void setTime(Date date) { this.time = date.getTime(); }
    public final Date getTime() { return new Date(time); }
    public void setTimeInMillis(long millis) { this.time = millis; }
    public long getTimeInMillis() { return time; }
    public void set(int field, int value) { }

    public int get(int field) {
        long secs = time / 1000;
        int ms = (int) (time % 1000);
        long days = secs / 86400;
        int sod = (int) (secs % 86400);
        int hour = sod / 3600, min = (sod % 3600) / 60, sec = sod % 60;
        int dow = (int) (((days % 7) + 4) % 7) + 1;
        long z = days + 719468;
        long era = (z >= 0 ? z : z - 146096) / 146097;
        long doe = z - era * 146097;
        long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        long y = yoe + era * 400;
        long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        long mp = (5 * doy + 2) / 153;
        int d = (int) (doy - (153 * mp + 2) / 5 + 1);
        int m = (int) (mp < 10 ? mp + 3 : mp - 9);
        if (m <= 2) y++;
        switch (field) {
            case YEAR:        return (int) y;
            case MONTH:       return m - 1;
            case DATE:        return d;
            case DAY_OF_WEEK: return dow;
            case HOUR_OF_DAY: return hour;
            case HOUR:        return hour % 12;
            case MINUTE:      return min;
            case SECOND:      return sec;
            case MILLISECOND: return ms;
            case AM_PM:       return hour < 12 ? AM : PM;
            case ERA:         return 1;
            default:          return 0;
        }
    }
}
