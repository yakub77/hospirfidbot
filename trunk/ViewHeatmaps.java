import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.applet.*;
import java.io.*;
import java.util.*;

public class ViewHeatmaps extends JFrame implements ActionListener {

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//A class for reading PGM IMages
	class PGMImage {
		public short[][] pixel;
		public int width, height, max;
		public String filename;
		//P5 Width Height Max <data...>
		public void interpretBytes(byte[] bytes) throws Exception {
			String str = new String(bytes);
			if (bytes[0] != 'P' || bytes[1] != '5')
				throw new Exception("Error: " + filename + " is not a valid PGM file");
			int i1 = 3;
			int i2 = str.indexOf(' ', i1);
			width = Integer.parseInt(str.substring(i1, i2));
			i1 = i2 + 1;
			i2 = str.indexOf(' ', i1);
			height = Integer.parseInt(str.substring(i1, i2));
			i1 = i2 + 1;
			if (!str.substring(i1, i1 + 3).equals("255")) {
				throw new Exception("Error: Max value is not 255; it is " + str.substring(i1, i1 + 3));
			}
			i1 += 3;
			while (str.charAt(i1) == ' ') i1++;
			pixel = new short[width][height];
			int imgindex = 0;
			for (int i = i1; i < bytes.length && imgindex < width * height; i++) {
				short val = bytes[i];
				if (val < 0) val += 256; //Two's complement
				pixel[imgindex % width][imgindex / width] = val;
				imgindex++;
			}
		}
		
		public PGMImage(String filename) throws Exception {
			File file = new File(filename);
			this.filename = filename;
			InputStream stream = new FileInputStream(file);
			long length = file.length();
			byte[] bytes = new byte[(int)length];
			int pos = 0;
			int numRead = 0;
			while (pos < bytes.length) {
				numRead = stream.read(bytes, pos, bytes.length - pos);
				pos += numRead;
				if (numRead < 0) break;
			}
			if (pos < bytes.length) {
				throw new IOException("Only read " + pos + " bytes out of " + bytes.length + " in file " + filename);
			}
			stream.close();
			interpretBytes(bytes);
		}
	}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
	
	public HashMap<String, PGMImage> heatmaps;
	//Used to store the "centroid" of each heatmap
	public HashMap<String, Integer> strongX;
	public HashMap<String, Integer> strongY;
	public PGMImage mapImage;
	public Display canvas;
	public JMenuBar menu;
	public String selected;
	
	public ViewHeatmaps(String mapfile, String prefix) throws Exception {
		super("Viewing heatmaps");

		//Set up the layout
		Container content = getContentPane();
		content.setLayout(new BorderLayout());
		canvas = new Display();
		content.add(canvas, BorderLayout.CENTER);
		
		menu=new JMenuBar();
		JMenu heatmapMenu=new JMenu("Select Tag");
		heatmapMenu.addActionListener(this);
		///////////
		
		//Load relevant image files
		File dir = new File("./");
		String[] dircontents = dir.list();
		mapImage = new PGMImage(mapfile);
		heatmaps = new HashMap<String, PGMImage>();
		strongX = new HashMap<String, Integer>();
		strongY = new HashMap<String, Integer>();
		
		for (int i = 0; i < dircontents.length; i++) {
			//If this is one of the heatmap image files
			int pgmindex = dircontents[i].indexOf(".pgm");
			if (dircontents[i].indexOf(prefix) == 0 && pgmindex > 0) {
				int num = Integer.parseInt(dircontents[i].substring(prefix.length(), pgmindex));
				PGMImage image = new PGMImage(dircontents[i]);
				heatmaps.put("" + num, image);
				heatmapMenu.add("" + num).addActionListener(this);
				//Find the "centroid" of this RFID heatmap and store it in strongX and strongY
				int strongest = 0;
				int x = -1, y = -1;
				for (int k = 0; k < image.width; k++) {
					for (int l = 0; l < image.height; l++) {
						if (image.pixel[k][l] > strongest) {
							x = k; y = l;
							strongest = image.pixel[k][l];
						}
					}
				}
				strongX.put("" + num, new Integer(x));
				strongY.put("" + num, new Integer(y));
			}
		}
		menu.add(heatmapMenu);
		content.add(menu, BorderLayout.NORTH);
		setSize(mapImage.width, mapImage.height);
		show();
	}
	
	//TODO: Add buffer images to make this faster
	class Display extends JPanel implements MouseListener, MouseMotionListener, MouseWheelListener {
		private boolean zooming;
		private int zoomX, zoomY;
		private int zoomFactor;
		private int centerX, centerY;
		
		public Display() {
			addMouseListener(this);
			addMouseMotionListener(this);
			addMouseWheelListener(this);
			zoomFactor = 1;
			centerX = -1; centerY = -1;
		}
	
		//Use the HSL color model to get a shade of orange,
		//then convert it to RGB
		double getRGBComponent(double p, double q, double tc) {
			if (tc < 1.0 / 6.0)
				return p + ((q-p)*6*tc);
			if (tc < 0.5)
				return q;
			if (tc < 2.0 / 3.0)
				return p + ((q-p)*6*(2.0/3.0 - tc));
			return p;
		}
		
		Color getColorHSL(double paramh, double params, double l) {
			double h = paramh / 256.0;
			double s = params / 256.0;
			double p, q;
			if (l < 0.5)	
				q = l * (l + s);
			else			
				q = l + s - (l*s);
			p = 2*l - q;
			double tr = h + 1.0 / 3.0;
			double tg = h;
			double tb = h - 1.0 / 3.0;
			if (tr < 0)	tr = tr + 1;if (tr > 1) tr = tr - 1;
			if (tg < 0)	tg = tg + 1;if (tg > 1) tg = tg - 1;
			if (tb < 0)	tb = tb + 1;if (tb > 1) tb = tb - 1;
			int r = (int)(255.0 * getRGBComponent(p, q, tr));
			int g = (int)(255.0 * getRGBComponent(p, q, tg));
			int b = (int)(255.0 * getRGBComponent(p, q, tb));
			return new Color(r, g, b);
		}
	
		public void paintComponent(Graphics g) {
			//System.out.println("painting\n");
			
			//First draw the map (occupancy grid) by itself
			int starti = 0, endi = mapImage.width;
			int startj = 0, endj = mapImage.height;
			if (zooming) { 
				starti = zoomX - mapImage.width / (2 * zoomFactor);
				endi = zoomX + mapImage.width / (2 * zoomFactor);
				startj = zoomY - mapImage.height / (2 * zoomFactor);
				endj = zoomY + mapImage.height / (2 * zoomFactor);
				//System.out.println(starti + "   " + endi);
				
				if (starti < 0) starti = 0;
				if (startj < 0) startj = 0;
				if (endi > mapImage.width - 1) endi = mapImage.width - 1;
				if (endj > mapImage.height - 1) endj = mapImage.height - 1;				
			}
			
			int pixelI = 0, pixelJ = 0;
			for (int i = starti; i < endi; i++) {
				pixelJ = 0;
				for (int j = startj; j < endj; j++) {
					int val = mapImage.pixel[i][j];
					g.setColor(new Color(val, val, val));
					g.fillRect(pixelI, pixelJ, zoomFactor, zoomFactor);
					pixelJ += zoomFactor;
					//System.out.println(pixelI + " , " + pixelJ);
				}
				pixelI += zoomFactor;
			}
			//Next draw the selected RFID heatmap over the map
			PGMImage heatmap = heatmaps.get(selected);			
			if (heatmap == null) return;
			Integer temp = strongX.get(selected);
			if (temp != null)
				centerX = temp.intValue();
			temp = strongY.get(selected);
			if (temp != null)
				centerY = temp.intValue();
			
			pixelI = 0;
			for (int i = starti; i < endi; i++) {
				pixelJ = 0;
				for (int j = startj; j < endj; j++) {
					int strength = heatmap.pixel[i][j];
					double l = ((double)strength + 50.0) / 200.0;
					if (strength > 10) {
						g.setColor(getColorHSL(20.0, 240.0, l));
						g.fillRect(pixelI, pixelJ, zoomFactor, zoomFactor);
					}
					pixelJ += zoomFactor;
				}
				pixelI += zoomFactor;
			}
			
			//Now draw the "centroid" as a red dot
			g.setColor(Color.RED);
			if (zooming) {
				int ovalX = heatmap.width / 2 + (centerX - zoomX) * zoomFactor;
				int ovalY = heatmap.height / 2 + (centerY - zoomY) * zoomFactor;
				int diameter = 10 * zoomFactor;
				g.fillOval(ovalX - diameter / 2, ovalY - diameter / 2, diameter, diameter);
			}
			else {
				g.fillOval(centerX - 5, centerY - 5, 10, 10);
			}
		}
		
		public void mouseEntered(MouseEvent evt) {}
		public void mouseExited(MouseEvent evt) {}
		public void mousePressed(MouseEvent evt) {
			zooming = true;
			zoomFactor = 2;
			zoomX = evt.getX();
			zoomY = evt.getY();
			repaint();
		}
		public void mouseReleased(MouseEvent evt) {
			zooming = false;
			zoomFactor = 1;
			repaint();
		}
		public void mouseClicked(MouseEvent evt) {}
		public void mouseMoved(MouseEvent evt){}
		public void mouseDragged(MouseEvent evt){
			zoomX = evt.getX();
			zoomY = evt.getY();
			//System.out.println(zoomX + " , " + zoomY);
			repaint();
		}
		public void mouseWheelMoved(MouseWheelEvent evt) {
			//System.out.println(evt.getWheelRotation());
			if (zooming) {
				zoomFactor -= evt.getWheelRotation();
				if (zoomFactor < 1) zoomFactor = 1;
				repaint();
			}
		}
	}
	
	public void actionPerformed(ActionEvent evt) {
		selected = evt.getActionCommand();
		repaint();
	}
	
	public static void main(String[] args) {
		if (args.length < 2) {
			System.err.println("Usage: ViewHeatmaps <map file> <heatmap_prefix>\n");
			return;
		}
		try {
			new ViewHeatmaps(args[0], args[1]);
		}
		catch (Exception e) { e.printStackTrace(); }
	}
}
