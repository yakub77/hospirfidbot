/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: Program takes a logfile of localized odometry data, an occupancy grid, 
 *a folder with JPEG frames from a webcam, and a file that gives timestamps 
 *for all of those frames.  It then advances through the frames in the left panel of
 *the display.  For each frame, find the system time it was taken, find the closest
 *system time to that time in the localized odometry logfile, and draw that pose
 *on an occupancy grid on the panel on the right of the display*/

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
import javax.imageio.ImageIO;  

public class ViewVideo extends JFrame implements WindowListener {
	public static double FPS = 10.0;//Frames per second
	
	public VideoDisplay videoDisplay;
	public MapDisplay mapDisplay;
	public double[] odomtimes;
	int size = 700;
	
	//Used to help find the "closest time" in the localized odometry logfile
	//This will return the index of the time less than or equal to the target
	//in the array a (and it runs in log(a.length) time by nature of the binary search)
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
	
	//This timer is used to trigger a frame refresh event to go to the next frame
	public Timer t = new Timer((int)(1000.0 / FPS), new ActionListener() {
		public void actionPerformed(ActionEvent e) {
			videoDisplay.nextFrame();//Go to the next video frame
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
		addWindowListener(this);
		Container content = getContentPane();
		content.setLayout(null);
		
		//Video display goes on the left
		videoDisplay = new VideoDisplay(cameralogfile, cameraprefix);
		videoDisplay.setBounds(0, 0, size, size);
		content.add(videoDisplay);
		//Map/odometry display goes on the right
		mapDisplay = new MapDisplay(mapfile, odomlogfile, gridscale);
		mapDisplay.setBounds(size, 0, 2*size, size);
		content.add(mapDisplay);
		setSize(2*size, size + 10);
		show();
		t.start();
	}
	
	class VideoDisplay extends JPanel {
		public String prefix;
		public double[] times;//The timestamps for all frames
		public double currTime;//The current time in the logfile
		public int index;
		public BufferedImage currFrame;
		
		//logfile stores the timestamps for all of the frames,
		//cameraprefix says what comes before the ".jpg" in all of the
		//frames
		public VideoDisplay(String logfile, String cameraprefix) {
			ArrayList<String> a = new ArrayList<String>();
			try {
				//Load in all of the timestamps for the frames specified
				//in "logfile"
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
		
		//Advance to the next frame
		public void nextFrame() {
			index++;
			index = index % times.length;//Take the modulus so that it loops
			//back when it gets to the end
			currTime = times[index];
			String filename = prefix + index + ".jpg";
			StringBuilder stringBuilder = new StringBuilder();
			Formatter formatter = new Formatter(stringBuilder);
			//Display the elapsed time from the first frame in parentheses in the title
			formatter.format("Video / Map Localization Viewer (Time: %.3f sec)", currTime - times[0]);
			setTitle(stringBuilder.toString());
			 
			//Load the image for the current frame
			try {    
				URL url = getClass().getResource(filename);  
				currFrame = ImageIO.read(url);  
			}  
			catch (Exception e) {
				e.printStackTrace();
			}
			if(currFrame == null) {  
				currFrame = new BufferedImage(400, 400, BufferedImage.TYPE_INT_RGB);  
				System.err.println("unable to load image, returning default");  
			}  
			repaint();
		}
		
		public void paintComponent(Graphics g) {
			g.drawImage(currFrame, 0, 0, size, size, null);
		}
	}
	
	class Odometry {
		public double x, y, theta;
		public Odometry(){}
	}
	
	class MapDisplay extends JPanel implements MouseListener, MouseMotionListener, MouseWheelListener {
		public TreeMap<Double, Odometry> odomReadings;//Associate an Odometry reading
		//with a system time
		public BufferedImage mapImage;//The stored occupancy grid image
		public double scale;
		public Odometry currentOdom = null;
		
		
		private int zoomX, zoomY;//How far from the center of the image the viewer is centered (in pixels)
		private int zoomFactor;//How many times magnified
		private int lastX, lastY;//Helps with scrolling
		private int currentX, currentY;//Used to help draw location in meters by the mouse pointer
	
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
       		
			addMouseListener(this);
			addMouseMotionListener(this);
			addMouseWheelListener(this);
			zoomFactor = 1;
			zoomX = 0;
			zoomY = 0;
		}
		
		public void paintComponent(Graphics g) {

			if (mapImage == null || currentOdom == null)
				return;
			int width = mapImage.getWidth();
			int height = mapImage.getHeight();
			Image image = createImage(width, height);
			Graphics ig = image.getGraphics();
			ig.drawImage(mapImage, 0, 0, width, height, null);
			ig.setColor(Color.RED);
			//Draw the robot with an arrow coming out of it in the proper direction
			//so that the user can tell orientation
			int x = mapImage.getWidth() / 2 + (int)(currentOdom.x / scale);
			int y = mapImage.getHeight() / 2 - (int)(currentOdom.y / scale);
			ig.fillOval(x - 4, y - 4, 8, 8);
			int x2 = x + (int)(16.0 * Math.cos(currentOdom.theta));
			int y2 = y - (int)(16.0 * Math.sin(currentOdom.theta));
			ig.drawLine(x, y, x2, y2);

			//Now draw that buffer at the appropriate position based on scrolling
			int drawX = width/2 - (width/2)*zoomFactor - zoomX * zoomFactor;
			int drawY = height/2 - (height/2)*zoomFactor - zoomY * zoomFactor;
			
			g.drawImage(image, drawX, drawY, width * zoomFactor, height * zoomFactor, this);
		}
		
		
		public void mouseEntered(MouseEvent evt) {}
		public void mouseExited(MouseEvent evt) {}
		public void mousePressed(MouseEvent evt) {
			lastX = evt.getX();
			lastY = evt.getY();
			repaint();
		}
		public void mouseReleased(MouseEvent evt) {
			repaint();
		}
		public void mouseClicked(MouseEvent evt) {}
		public void mouseMoved(MouseEvent evt){
			currentX = evt.getX();
			currentY = evt.getY();
			repaint();
		}
		public void mouseDragged(MouseEvent evt){
			int x = evt.getX(), y = evt.getY();
			int dx = x - lastX;
			int dy = y - lastY;
			zoomX -= dx;
			zoomY -= dy;
			lastX = x;
			lastY = y;
			repaint();
		}
		public void mouseWheelMoved(MouseWheelEvent evt) {
			zoomFactor -= evt.getWheelRotation();
			if (zoomFactor < 1) zoomFactor = 1;
			repaint();
		}
	}
	
	
	public void windowOpened(java.awt.event.WindowEvent evt){}
	public void windowClosing(java.awt.event.WindowEvent evt){
		t.stop();
		System.exit(0);
	}
	public void windowClosed(java.awt.event.WindowEvent evt){}
	public void windowActivated(java.awt.event.WindowEvent evt){}
	public void windowDeactivated(java.awt.event.WindowEvent evt){}
	public void windowIconified(java.awt.event.WindowEvent evt){}
	public void windowDeiconified(java.awt.event.WindowEvent evt){}
	
	
	public static void main(String[] args) {
		if (args.length < 6) {
			System.err.println("Usage: VideoDisplay <map file> <map grid scale (meters)> <localized odometry logfile> <camera logfile> <camera directory/prefix> <framerate>\n");
			return;
		}
		FPS = Double.parseDouble(args[5]);
		new ViewVideo(args[0], Double.parseDouble(args[1]), args[2], args[3], args[4]);
	}
}
