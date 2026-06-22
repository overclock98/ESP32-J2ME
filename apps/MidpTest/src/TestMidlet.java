import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;
import javax.microedition.midlet.MIDlet;

/* Hand-written MIDlet that exercises the shim exactly like the game will:
 * startApp() -> Display.setCurrent(canvas) -> a game-loop thread that mutates state
 * and calls repaint() -> Canvas.paint(Graphics) -> flint.drawing -> Lcd.present(). */
public class TestMidlet extends MIDlet {
    private DemoCanvas canvas;

    protected void startApp() {
        System.out.println("startApp");
        canvas = new DemoCanvas();
        Display.getDisplay(this).setCurrent(canvas);
        new Thread(canvas).start();
    }
    protected void pauseApp() {}
    protected void destroyApp(boolean unconditional) {}
}

class DemoCanvas extends Canvas implements Runnable {
    private int x = 10, dir = 7;
    private long frames = 0;
    private final Image tile;     // mutable image drawn via getGraphics()
    private final Image sprite;   // ARGB image with transparency (createRGBImage)
    private String resStatus;     // getResourceAsStream("/cr.f") result, shown each log line

    DemoCanvas() {
        setFullScreenMode(true);

        /* resource-loading check (printed in the loop so it's visible despite USB reconnects) */
        try (java.io.InputStream is = getClass().getResourceAsStream("/cr.f")) {
            resStatus = (is == null) ? "cr.f=NULL" : ("cr.f=" + is.readAllBytes().length + "B");
        } catch (Exception e) {
            resStatus = "cr.f=ERR:" + e;
        }

        // mutable image: draw into it once via getGraphics(), then blit it each frame
        tile = Image.createImage(64, 64);
        Graphics tg = tile.getGraphics();
        tg.setColor(0xE08020); tg.fillRect(0, 0, 64, 64);
        tg.setColor(0xFFFFFF); tg.drawRect(0, 0, 63, 63);
        tg.setColor(0x202020); tg.fillRect(16, 16, 32, 32);

        // ARGB sprite: a yellow diamond on a transparent background
        int s = 48, cx = s / 2;
        int[] argb = new int[s * s];
        for (int yy = 0; yy < s; yy++)
            for (int xx = 0; xx < s; xx++)
                argb[yy * s + xx] = (Math.abs(xx - cx) + Math.abs(yy - cx) < cx)
                        ? 0xFFFFEE33 : 0x00000000;
        sprite = Image.createRGBImage(argb, s, s, true);
    }

    protected void paint(Graphics g) {
        int w = getWidth(), h = getHeight();
        g.setColor(0x000028); g.fillRect(0, 0, w, h);            // bg
        g.setColor(0x1EA05A); g.fillRect(0, 0, w, 46);           // top bar
        g.setColor(0xFFFFFF); g.drawRect(6, 60, w - 12, h - 70); // border
        g.setColor(0xDC3C32); g.fillRect(x, h / 2 - 45, 90, 90); // moving box

        g.drawImage(tile, 20, 80, Graphics.TOP | Graphics.LEFT);          // static mutable image
        g.drawImage(sprite, x + 45, h / 2, Graphics.HCENTER | Graphics.VCENTER); // sprite rides the box, alpha-blended
    }

    public void run() {
        int w = getWidth();
        while (true) {
            x += dir;
            if (x < 6 || x > w - 96) dir = -dir;
            repaint();
            if ((frames++ & 31) == 0) System.out.println("frame " + frames + " " + resStatus);
            try { Thread.sleep(20); } catch (InterruptedException e) {}
        }
    }
}
