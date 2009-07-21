import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import javax.swing.*;
import javax.swing.Timer;
import javax.swing.event.*;
import java.applet.*;
import java.io.*;
import java.util.*;
import java.net.URL;

/*Steps:
1: Read in log files for the odometry data and for the image times
2: 

*/

public class ViewVideo extends JFrame implements WindowListener {
	public static double FPS = 10.0;//Frames per second
	
	public VideoDisplay videoDisplay;
	public MapDisplay mapDisplay;
	public double[] odomtimes;
	
	public static int binarySearch(double[] a, double target, int begin, int end) {
		int mid = (begin + end) / 2;
		if (a[mid] == target || begin > end)
			return mid;
		if (target > a[mid]) {
			return binarySearch(a, target, mid + 1, end);
		}
		else {
			return binarySearch(a, target, begin, mid - 1);
		}
	}
	
	Timer t = new Timer((int)(1000.0 / FPS), new ActionListener() {
		public void actionPerformed(ActionEvent e) {
			videoDisplay.nextFrame();
			Double time = new Double(videoDisplay.currTime);
			double timeval = videoDisplay.currTime;
			//public TreeMap<Double, Odometry> odomReadings
			if (mapDisplay.odomReadings.keySet().contains(time)) {
				mapDisplay.currentOdom = mapDisplay.odomReadings.get(time);
			}
			else {
				//Find the nearest odometry reading based on time
				//by doing a binary search
				int index = binarySearch(odomtimes, timeval, 0, odomtimes.length - 1);
				if (index == odomtimes.length - 1) {
					mapDisplay.currentOdom = mapDisplay.odomReadings.get(new Double(odomtimes[index]));
				}
				else {
					int begin = index;
					int end = index + 1;
					if ((timeval - odomtimes[begin]) < (odomtimes[end] - timeval)) {
						mapDisplay.currentOdom = mapDisplay.odomReadings.get(
							new Double(odomtimes[begin]));
					}
					else {
						mapDisplay.currentOdom = mapDisplay.odomReadings.get(
							new Double(odomtimes[end]));
					}
				}
			}
			mapDisplay.repaint();
		}
	});
	
	public ViewVideo(String mapfile, double gridscale, String odomlogfile, String cameralogfile, String cameraprefix) {
		super("Video / Map Localization Viewer");
		Container content = getContentPane();
		content.setLayout(null);
		
		videoDisplay = new VideoDisplay(cameralogfile, cameraprefix);
		videoDisplay.setBounds(0, 0, 400, 400);
		content.add(videoDisplay);
		mapDisplay = new MapDisplay(mapfile, odomlogfile, gridscale);
		mapDisplay.setBounds(400, 0, 800, 400);
		content.add(mapDisplay);
		setSize(800, 400);
		show();
	}
	
	
	class VideoDisplay extends JPanel {
		public String prefix;
		public double[] times;
		public double currTime;//The current time in the logfile
		public int index;
		public Image currFrame;
		
		public VideoDisplay(String logfile, String cameraprefix) {
			ArrayList<String> a = new ArrayList<String>();
			try {
				FileInputStream fstream = new FileInputStream(logfile);
				DataInputStream in = new DataInputStream(fstream);
				prefix = cameraprefix;
			        while (in.available() != 0) {
		               		String str = in.readLine();
		       			if (str.length() != 0) {
		       				if (str.charAt(0) == '#') continue;
		       				a.add(str);
		       			}
		       		}
               		}
               		catch (Exception e) {
               			e.printStackTrace();
               		}
               		times = new double[a.size()];
               		if (a.size() < 1) {
               			System.err.println("Error: No video frames to display");
               			System.exit(0);
               		}
               		for (int i = 0; i < a.size(); i++) {
               			times[i] = Double.parseDouble(a.get(i));
               		}
               		index = 0;
		}
		
		public void nextFrame() {
			index++;
			index = index % times.length;
			currTime = times[index];
			String filename = prefix + index + ".jpg";
			URL url = this.getClass().getResource(filename);
			currFrame = Toolkit.getDefaultToolkit().getImage(url);
			repaint();
		}
		
		public void paintComponent(Graphics g) {
			g.drawImage(currFrame, 0, 0, 400, 400, null);
		}
	}
	
	class Odometry {
		public double x, y, theta;
		public Odometry(){}
	}
	
	class MapDisplay extends JPanel {
		public TreeMap<Double, Odometry> odomReadings;
		public BufferedImage mapImage;
		public double scale;
		public Odometry currentOdom = null;
	
		public MapDisplay(String mapfile, String logfile, double scale) {
			PGMImage image = null;
			try {	
				image = new PGMImage(mapfile);
			}
			catch (Exception e) {e.printStackTrace();}
			mapImage = image.getBufferedImage();
			odomReadings = new TreeMap<Double, Odometry>();
			this.scale = scale;
			try {
				FileInputStream fstream = new FileInputStream(logfile);
				DataInputStream in = new DataInputStream(fstream);
			        while (in.available() != 0) {
		               		String str = in.readLine();
		       			if (str.length() != 0) {
		       				if (str.charAt(0) == '#') continue;
		       				StringTokenizer tokenizer = new StringTokenizer(str, " ");
		       				//1248122145.715 16777343 6665 position2d 01 001 001 +17.958 +01.742 +1.137 +00.010
		       				Double time = new Double(tokenizer.nextToken());
		       				//<device num> <port> <interface name>
		       				tokenizer.nextToken();tokenizer.nextToken();tokenizer.nextToken();
		       				//<junk> <junk> <junk>
		       				tokenizer.nextToken();tokenizer.nextToken();tokenizer.nextToken();
		       				//<x> <y> <theta>
		       				Odometry odom = new Odometry();
		       				odom.x = Double.parseDouble(tokenizer.nextToken());
		       				odom.y = Double.parseDouble(tokenizer.nextToken());
		       				odom.theta = Double.parseDouble(tokenizer.nextToken());
		       				odomReadings.put(time, odom);
		       			}
		       		}
               		}
               		catch (Exception e) {
               			e.printStackTrace();
               		}
               		odomtimes = new double[odomReadings.size()];
               		Iterator<Double> iter = odomReadings.keySet().iterator();
               		int index = 0;
               		while (iter.hasNext()) {
               			odomtimes[index] = iter.next();
               			index++;
               		}
		}
		
		public void paintComponent(Graphics g) {
			int width = mapImage.getWidth();
			int height = mapImage.getHeight();
			g.drawImage(mapImage, 0, 0, 400, 400, null);
			int x = mapImage.getWidth() / 2 + (int)(currentOdom.x / scale);
			int y = mapImage.getHeight() / 2 - (int)(currentOdom.y / scale);
			x = (int)((double)x * 400.0 / (double)width);
			y = (int)((double)y * 400.0 / (double)height);
			g.setColor(Color.RED);
			g.fillOval(x - 2, y - 2, 4, 4);
			int x2 = x + (int)(6.0 * Math.cos(currentOdom.theta));
			int y2 = y + (int)(6.0 * Math.sin(currentOdom.theta));
			g.drawLine(x, y, x2, y2);
		}
	}
	
	
	public void windowOpened(java.awt.event.WindowEvent evt){}
	public void windowClosing(java.awt.event.WindowEvent evt){
		System.exit(0);
	}
	public void windowClosed(java.awt.event.WindowEvent evt){}
	public void windowActivated(java.awt.event.WindowEvent evt){}
	public void windowDeactivated(java.awt.event.WindowEvent evt){}
	public void windowIconified(java.awt.event.WindowEvent evt){}
	public void windowDeiconified(java.awt.event.WindowEvent evt){}
	
	
	public static void main(String[] args) {
		if (args.length < 5) {
			System.err.println("Usage: VideoDisplay <map file> <map grid scale (meters)> <localized odometry logfile> <camera logfile> <camera directory/prefix>\n");
			return;
		}
		new ViewVideo(args[0], Double.parseDouble(args[1]), args[2], args[3], args[4]);
	}
}
