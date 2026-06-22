public class Main {
    public static void main(String[] args) {
        System.out.println(">>> FlintESPJVM up on ESP32-S3 <<<");
        int s = 0;
        for (int i = 1; i <= 100; i++) s += i;
        System.out.println("sum 1..100 = " + s);
    }
}
