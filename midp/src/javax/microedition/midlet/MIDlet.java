package javax.microedition.midlet;

/* Minimal MIDP MIDlet base. The launcher (default-package Main) calls startApp0()
 * to enter the MIDlet — startApp() itself is protected per the spec. */
public abstract class MIDlet {
    protected MIDlet() {}

    protected abstract void startApp() throws MIDletStateChangeException;
    protected abstract void pauseApp();
    protected abstract void destroyApp(boolean unconditional) throws MIDletStateChangeException;

    /** Launcher entry point (not part of the MIDP API). */
    public final void startApp0() throws MIDletStateChangeException { startApp(); }

    public final void notifyDestroyed() { /* no app-manager to return to */ }
    public final void notifyPaused() {}
    public final void resumeRequest() {}
    public final boolean platformRequest(String url) { return false; }
    public final int checkPermission(String permission) { return -1; }

    /* App properties come from the game's manifest, bundled to /res/MANIFEST.MF. */
    private static java.util.Hashtable<String, String> props;
    public final String getAppProperty(String key) {
        if (props == null) loadProps();
        return props.get(key);
    }
    private static synchronized void loadProps() {
        java.util.Hashtable<String, String> p = new java.util.Hashtable<>();
        try (java.io.InputStream is = MIDlet.class.getResourceAsStream("/MANIFEST.MF")) {
            if (is != null) {
                String s = new String(is.readAllBytes());
                int start = 0;
                while (start < s.length()) {
                    int nl = s.indexOf('\n', start);
                    if (nl < 0) nl = s.length();
                    String line = s.substring(start, nl);
                    int colon = line.indexOf(':');
                    if (colon > 0) p.put(line.substring(0, colon).trim(), line.substring(colon + 1).trim());
                    start = nl + 1;
                }
            }
        } catch (Exception e) { /* fall back to empty */ }
        props = p;
    }
}
