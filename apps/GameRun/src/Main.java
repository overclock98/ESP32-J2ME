import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.midlet.MIDlet;

/* Launcher for Gameloft Diamond Rush (S40 v5): bring up the screen, instantiate the MIDlet
 * (GloftDIRU per the jar manifest), and enter startApp(). The GloftDIRU ctor creates the
 * GameCanvas `i`; startApp() setCurrent()s it and calls i.f() which starts the run() loop.
 *
 * Input is the on-screen touch keypad (board.Touch): the game renders the top 320x360, and
 * the bottom strip is a 7 8 9 / 4 5 6 / 1 2 3 / * 0 # keypad drawn natively. Serial-key input
 * has been removed. */
public class Main {
    public static void main(String[] args) throws Exception {
        System.out.println("Diamond Rush: boot");
        Display.initScreen();
        board.Touch.init();          // I2C touch + draw the on-screen keypad in the bottom strip
        System.out.println("screen up; creating MIDlet GloftDIRU");
        MIDlet m = new GloftDIRU();  // Gameloft Diamond Rush MIDlet (S40 v5)
        System.out.println("startApp...");
        m.startApp0();
        System.out.println("startApp returned; entering park loop");

        /* Touch-keypad input is delivered on the GAME thread inside GameCanvas.flushGraphics()
         * (see midp GameCanvas.pumpTouch) — no separate input thread, so nothing competes with
         * or starves the game's render loop. Nothing to do here but keep the VM alive. */

        int s = 0;
        while (true) { Thread.sleep(1000); System.out.println("alive " + (++s) + "s"); }
    }
}
