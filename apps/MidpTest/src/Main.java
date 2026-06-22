import javax.microedition.lcdui.Display;
import javax.microedition.midlet.MIDlet;

/* Generic MIDlet launcher: bring up the screen, instantiate the MIDlet, enter startApp().
 * For the real game this instantiates GloftDIRU; here it runs the bring-up TestMidlet. */
public class Main {
    public static void main(String[] args) throws Exception {
        System.out.println("midp test: boot");
        Display.initScreen();
        System.out.println("screen " + javax.microedition.lcdui.Display.getDisplay(null));
        MIDlet midlet = new TestMidlet();
        midlet.startApp0();
        /* park main; the MIDlet started its own game-loop thread */
        while (true) Thread.sleep(1000);
    }
}
