import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import java.applet.*;
import java.io.*;
import java.util.*;

public class ViewHeatmaps extends JFrame implements ListSelectionListener, WindowListener {
	class RFIDTableEntry {
		/*fprintf(fp, "%i %s %i %lf %lf\n", iter->first, rfidlog->reverseTags[iter->first].data(), 
			iter->second.strength, iter->second.x, iter->second.y);*/
		public String id;
		public String hexID;
		public int strength;
		public double x, y;
		public String toString() {
			return (id + "," + hexID + "," + strength + "," + x + "," + y);
		}
		public Vector asVector() {
			Vector toReturn = new Vector();
			toReturn.add("" + id);
			toReturn.add(hexID);
			toReturn.add("" + strength);
			toReturn.add("" + x + "m");
			toReturn.add("" + y + "m");
			return toReturn;
		}
	}
	
	
	public TreeMap<String, Image> heatmaps;
	public TreeMap<String, RFIDTableEntry> tableEntries;
	//Used to store the "centroid" of each heatmap
	public Image mapImage;
	public Display canvas;
	public JScrollPane scrollPane;
	public JTable table;
	public String selected = "";
	public double mapRes;
	public String fileprefix;
	
	
	public void loadHeatmap(String id) {
		Graphics mapGraphics = mapImage.getGraphics();
		mapGraphics.setColor(Color.BLUE);
		try {
			String filename = fileprefix + id + ".pgm";
			PGMImage pgmimage = new PGMImage(filename);
			Image image = pgmimage.getBufferedHeatmap();
			//Draw the dot on the centroid
			RFIDTableEntry entry = tableEntries.get(id);
			int x = (int)(entry.x / mapRes) + pgmimage.width / 2;
			int y = pgmimage.height / 2 - (int)(entry.y / mapRes);
			Graphics g = image.getGraphics();
			g.setColor(Color.RED);
			g.fillOval(x - 3, y - 3, 6, 6);
			mapGraphics.fillOval(x - 2, y - 2, 4, 4);
			heatmaps.put(id, image);
			//System.out.println("Finished loading " + filename + "...");
		}
		catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	//Initialize the table with all of the "centroid" information for each tag
	public void fillTable(String centroidsFile, boolean preload) throws Exception {
                FileInputStream fstream = new FileInputStream(centroidsFile);
		DataInputStream in = new DataInputStream(fstream);
		Vector rowData = new Vector();
		
		//Read in the file with all of the table info
		mapRes = Double.parseDouble(in.readLine());		
                while (in.available() != 0) {
                        String str = in.readLine();
                        StringTokenizer tokenizer = new StringTokenizer(str, " ");
                        RFIDTableEntry entry = new RFIDTableEntry();
			entry.id = tokenizer.nextToken();
			entry.hexID = tokenizer.nextToken();
			entry.strength = Integer.parseInt(tokenizer.nextToken());
			entry.x = Double.parseDouble(tokenizer.nextToken());
			entry.y = Double.parseDouble(tokenizer.nextToken());
			tableEntries.put(entry.id, entry);
			rowData.add(entry.asVector());
			if (preload) {
				loadHeatmap("" + entry.id);
			}
			//System.out.println(entry);
		}
		//Write all of the info to the table object
		Vector columnNames = new Vector(Arrays.asList(new String[]{"ID", "Hex ID", "Strength", "Centroid_X", "Centroid_Y"}));
		table = new JTable(rowData, columnNames);
		scrollPane = new JScrollPane(table);
		in.close();
	}
	
	public ViewHeatmaps(String mapfile, String prefix, String centroidsFile, boolean preload) throws Exception {
		super("Viewing heatmaps");

		fileprefix = prefix;
		//Set up the layout
		Container content = getContentPane();
		content.setLayout(null);
		//////////
		PGMImage mapImagePGM = new PGMImage(mapfile);
		mapImage = mapImagePGM.getBufferedImage();
		heatmaps = new TreeMap<String, Image>();
		tableEntries = new TreeMap<String, RFIDTableEntry>();
		fillTable(centroidsFile, preload);
		if (tableEntries.size() > 0) {
			//Do a default selection so the user knows they can use the table
			selected = table.getValueAt(0, 0).toString();
			table.addRowSelectionInterval(0, 0);
		}
		table.getSelectionModel().addListSelectionListener(this);
		scrollPane.setBounds(0, 0, mapImage.getWidth(this), 100);
		content.add(scrollPane);
		canvas = new Display();
		canvas.setBounds(0, 100, mapImage.getWidth(this), mapImage.getHeight(this) + 100);
		content.add(canvas);
		setSize(mapImage.getWidth(this), mapImage.getHeight(this) + 100);
		addWindowListener(this);
		show();
	}
	
	public void valueChanged(ListSelectionEvent evt) {
		int row = table.getSelectedRow();
		selected = table.getValueAt(row, 0).toString();
		if (heatmaps.get(selected) == null) {
			//The heatmap has not been loaded yet, so load it
			loadHeatmap(selected);
		}
		repaint();
	}
	
	
	class Display extends JPanel implements MouseListener, MouseMotionListener, MouseWheelListener {
		private int zoomX, zoomY;//How far from the center of the image the viewer is centered (in pixels)
		private int zoomFactor;//How many times magnified
		private int width, height;
		private int lastX, lastY;//Helps with scrolling
		private int currentX, currentY;//Used to help draw location in meters by the mouse pointer
		
		public Display() {
			width = mapImage.getWidth(this);
			height = mapImage.getHeight(this);
			addMouseListener(this);
			addMouseMotionListener(this);
			addMouseWheelListener(this);
			zoomFactor = 1;
			zoomX = 0;
			zoomY = 0;
		}
	
		public void paintComponent(Graphics g) {
			g.setColor(Color.BLACK);
			g.fillRect(0, 0, width, height);
			
			int x = width/2 - (width/2)*zoomFactor - zoomX * zoomFactor;
			int y = height/2 - (height/2)*zoomFactor - zoomY * zoomFactor;
			
			g.drawImage(mapImage, x, y, width * zoomFactor, height * zoomFactor, this);
			Image heatmap = heatmaps.get(selected);
			if (heatmap != null)
				g.drawImage(heatmap, x, y, width * zoomFactor, height * zoomFactor, this);
				
			//Draw the location in meters
			//Go into a new coordinate system based on where the image is centered
			int vx = (currentX - width/2) / zoomFactor, vy = (height/2 - currentY) / zoomFactor;
			int ax = zoomX + vx, ay = vy - zoomY;
			double xloc = (double)ax * mapRes;
			double yloc = (double)ay * mapRes;
			g.setColor(Color.BLUE);
			StringBuilder stringBuilder = new StringBuilder();
			Formatter formatter = new Formatter(stringBuilder);
			formatter.format("(%.2f, %.2f)", xloc, yloc);
			g.drawString(stringBuilder.toString(), currentX, currentY);
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
		System.exit(0);
	}
	public void windowClosed(java.awt.event.WindowEvent evt){}
	public void windowActivated(java.awt.event.WindowEvent evt){}
	public void windowDeactivated(java.awt.event.WindowEvent evt){}
	public void windowIconified(java.awt.event.WindowEvent evt){}
	public void windowDeiconified(java.awt.event.WindowEvent evt){}
	
	public static void main(String[] args) {
		if (args.length < 4) {
			System.err.println("Usage: ViewHeatmaps <map file> <heatmap_prefix> <centroids> <preload>\n");
			return;
		}
		try {
			new ViewHeatmaps(args[0], args[1], args[2], Boolean.parseBoolean(args[3]));
		}
		catch (Exception e) { e.printStackTrace(); }
	}
}
