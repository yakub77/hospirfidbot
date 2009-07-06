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
		
		for (int i = 0; i < dircontents.length; i++) {
			//If this is one of the heatmap image files
			int pgmindex = dircontents[i].indexOf(".pgm");
			if (dircontents[i].indexOf(prefix) == 0 && pgmindex > 0) {
				int num = Integer.parseInt(dircontents[i].substring(prefix.length(), pgmindex));
				PGMImage image = new PGMImage(dircontents[i]);
				heatmaps.put("" + num, image);
				heatmapMenu.add("" + num).addActionListener(this);
			}
		}
		menu.add(heatmapMenu);
		content.add(menu, BorderLayout.NORTH);
		setSize(mapImage.width, mapImage.height);
		show();
	}
	
	class Display extends JPanel {
		public void paintComponent(Graphics g) {
			//System.out.println("painting\n");
			//First draw the map (occupancy grid) by itself
			for (int i = 0; i < mapImage.width; i++) {
				for (int j = 0; j < mapImage.height; j++) {
					int val = mapImage.pixel[i][j];
					g.setColor(new Color(val, val, val));
					g.fillRect(i, j, 1, 1);
				}
			}
			//Next draw the selected RFID heatmap over the map
			PGMImage heatmap = heatmaps.get(selected);
			if (heatmap == null) return;
			
			for (int i = 0; i < heatmap.width; i++) {
				for (int j = 0; j < heatmap.height; j++) {
					if (heatmap.pixel[i][j] > 127) {
						int val = heatmap.pixel[i][j];
						if (val < 150)	g.setColor(Color.BLUE);
						else if (val < 170) g.setColor(Color.GREEN);
						else if (val < 190) g.setColor(Color.YELLOW);
						else if (val < 210) g.setColor(Color.ORANGE);
						else	g.setColor(Color.RED);
						g.fillRect(i, j, 1, 1);
					}
				}
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
