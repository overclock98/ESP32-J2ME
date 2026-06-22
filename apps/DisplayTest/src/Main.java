import flint.drawing.Graphics;
import flint.drawing.Color;

/* Display bring-up test: render an animated frame through flint.drawing into a
 * screen-sized RGB565 buffer and push it to the panel via Lcd.present(). The box
 * bounces left-right; re-presenting every frame also keeps the AXS15231B from
 * blanking itself (~700 ms idle). */
public class Main {
    public static void main(String[] args) throws Exception {
        System.out.println("display test: init lcd");
        Lcd.init();
        int w = Lcd.width();
        int h = Lcd.height();
        System.out.println("lcd " + w + "x" + h);

        byte[] fb = new byte[w * h * 2];
        Graphics g = Graphics.create(w, h, fb);

        Color bg    = new Color(0, 0, 40);
        Color bar   = new Color(30, 160, 90);
        Color box   = new Color(220, 60, 50);
        Color frame = new Color(255, 255, 255);

        int boxW = 90, boxH = 90;
        int x = 10, dir = 7;
        long frames = 0;

        while (true) {
            g.clear(bg);
            g.fillRect(bar, 0, 0, w, 46);                       // top bar
            g.drawRect(frame, 3, 6, 60, w - 12, h - 70);        // border
            g.fillRect(box, x, h / 2 - boxH / 2, boxW, boxH);   // moving box
            Lcd.present(fb);

            x += dir;
            if (x < 6 || x > w - boxW - 6) dir = -dir;

            if ((frames++ & 31) == 0)
                System.out.println("frame " + frames);
            Thread.sleep(20);
        }
    }
}
